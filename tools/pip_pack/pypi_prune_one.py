"""Delete one old PyPI release before publishing, to stay under the project quota.

PyPI has no API for deleting releases. API tokens and trusted publishers can
only upload, and yanking a release does not free storage. This script logs in
through the website and deletes a single release.

Selection, oldest upload time first:

1. A beta (PEP 440 ``b``) uploaded more than 30 days ago.
2. Otherwise the oldest release of any kind.

The version about to be uploaded is never deleted. Deletion is permanent.
"""

from __future__ import annotations

import argparse
import email
import imaplib
import os
import re
import sys
import time
from dataclasses import dataclass
from datetime import datetime, timedelta, timezone
from email.utils import parsedate_to_datetime
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import quote, urlparse

from packaging.utils import parse_wheel_filename
from packaging.version import InvalidVersion, Version

BETA_MIN_AGE = timedelta(days=30)
INDEX_JSON = "https://pypi.org/pypi/{package}/json"
CONFIRM_URL_RE = re.compile(
    r"https://pypi\.org/account/confirm-login/\?token=[A-Za-z0-9._~%-]+"
)
USER_AGENT = "MaaFramework-pypi-prune (+https://github.com/MaaXYZ/MaaFramework)"


class PruneError(RuntimeError):
    pass


@dataclass(frozen=True)
class Release:
    version: str
    uploaded_at: datetime


@dataclass(frozen=True)
class Choice:
    release: Release
    reason: str


@dataclass(frozen=True)
class ImapConfig:
    host: str
    user: str
    password: str
    mailbox: str
    port: int


def is_beta(version: str) -> bool:
    try:
        parsed = Version(version)
    except InvalidVersion:
        return False
    return parsed.pre is not None and parsed.pre[0] == "b"


def choose_victim(
    releases: list[Release],
    *,
    protect: set[str],
    now: datetime,
) -> Choice | None:
    eligible = [item for item in releases if item.version not in protect]
    if not eligible:
        return None

    cutoff = now - BETA_MIN_AGE
    old_betas = [
        item for item in eligible if is_beta(item.version) and item.uploaded_at < cutoff
    ]
    if old_betas:
        victim = min(old_betas, key=lambda item: (item.uploaded_at, item.version))
        return Choice(victim, f"oldest beta older than {BETA_MIN_AGE.days} days")

    victim = min(eligible, key=lambda item: (item.uploaded_at, item.version))
    return Choice(victim, "oldest release")


def load_releases(payload: dict) -> list[Release]:
    releases: list[Release] = []
    for version, files in payload.get("releases", {}).items():
        uploaded_at = _earliest_upload(files)
        if uploaded_at is None:
            continue
        releases.append(Release(str(version), uploaded_at))
    return releases


def _earliest_upload(files: list[dict]) -> datetime | None:
    timestamps: list[datetime] = []
    for info in files:
        raw = info.get("upload_time_iso_8601") or ""
        if not raw:
            continue
        timestamps.append(datetime.fromisoformat(raw.replace("Z", "+00:00")))
    if not timestamps:
        return None
    return min(timestamps)


def published_filenames(payload: dict) -> set[str]:
    names: set[str] = set()
    for files in payload.get("releases", {}).values():
        for info in files:
            filename = info.get("filename")
            if filename:
                names.add(filename)
    return names


def local_wheels(directory: Path) -> list[Path]:
    return sorted(path for path in directory.rglob("*.whl") if path.is_file())


def wheel_versions(wheels: list[Path]) -> set[str]:
    versions: set[str] = set()
    for wheel in wheels:
        _name, version, _build, _tags = parse_wheel_filename(wheel.name)
        versions.add(str(version))
    return versions


class _FormParser(HTMLParser):
    def __init__(self, required_input: str):
        super().__init__()
        self._required_input = required_input
        self.csrf: str | None = None
        self._in_form = False
        self._csrf: str | None = None
        self._matched = False

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        attr = dict(attrs)
        if tag == "form":
            self._in_form = True
            self._csrf = None
            self._matched = False
            return
        if not self._in_form or tag != "input":
            return
        name = attr.get("name")
        if name == "csrf_token" and attr.get("value"):
            self._csrf = attr["value"]
        if name == self._required_input:
            self._matched = True

    def handle_endtag(self, tag: str) -> None:
        if tag != "form" or not self._in_form:
            return
        self._in_form = False
        if self._matched and self._csrf and self.csrf is None:
            self.csrf = self._csrf


def csrf_for_input(html: str, required_input: str) -> str | None:
    parser = _FormParser(required_input)
    parser.feed(html)
    return parser.csrf


def _env(name: str) -> str:
    return os.environ.get(name, "").strip()


def load_imap_config() -> ImapConfig | None:
    host = _env("PYPI_IMAP_HOST")
    user = _env("PYPI_IMAP_USER")
    password = _env("PYPI_IMAP_PASSWORD")
    provided = [bool(host), bool(user), bool(password)]
    if not any(provided):
        return None
    if not all(provided):
        raise PruneError(
            "PYPI_IMAP_HOST, PYPI_IMAP_USER and PYPI_IMAP_PASSWORD must be set together"
        )

    port_raw = _env("PYPI_IMAP_PORT") or "993"
    try:
        port = int(port_raw)
    except ValueError as exc:
        raise PruneError(f"PYPI_IMAP_PORT is not an integer: {port_raw}") from exc

    return ImapConfig(
        host=host,
        user=user,
        password=password,
        mailbox=_env("PYPI_IMAP_MAILBOX") or "INBOX",
        port=port,
    )


def require_login_env() -> tuple[str, str, str]:
    username = _env("PYPI_USERNAME")
    password = _env("PYPI_PASSWORD")
    totp_secret = "".join(_env("PYPI_TOTP_SECRET").split())
    missing = [
        name
        for name, value in (
            ("PYPI_USERNAME", username),
            ("PYPI_PASSWORD", password),
            ("PYPI_TOTP_SECRET", totp_secret),
        )
        if not value
    ]
    if missing:
        raise PruneError(
            "Missing PyPI login secrets: "
            + ", ".join(missing)
            + ". Trusted publishing cannot delete releases; "
            "create a dedicated PyPI user that only maintains this project "
            "and store its username, password and TOTP seed."
        )
    return username, password, totp_secret


def _needs_device_confirm(url: str, html: str) -> bool:
    path = urlparse(url).path
    if path.rstrip("/").endswith("/account/confirm-login"):
        return True
    lowered = html.lower()
    return "confirm this login" in lowered or "unrecognized device" in lowered


def _confirm_url_from_message(raw: bytes) -> str | None:
    message = email.message_from_bytes(raw)
    chunks: list[str] = []
    if message.is_multipart():
        for part in message.walk():
            if part.get_content_type() not in {"text/plain", "text/html"}:
                continue
            payload = part.get_payload(decode=True)
            if payload:
                chunks.append(payload.decode(part.get_content_charset() or "utf-8", "replace"))
    else:
        payload = message.get_payload(decode=True)
        if payload:
            chunks.append(payload.decode(message.get_content_charset() or "utf-8", "replace"))

    match = CONFIRM_URL_RE.search(" ".join(chunks))
    if match is None:
        return None
    return match.group(0)


def _message_datetime(raw: bytes) -> datetime | None:
    message = email.message_from_bytes(raw)
    raw_date = message.get("Date")
    if not raw_date:
        return None
    try:
        parsed = parsedate_to_datetime(raw_date)
    except (TypeError, ValueError, IndexError):
        return None
    if parsed.tzinfo is None:
        return parsed.replace(tzinfo=timezone.utc)
    return parsed.astimezone(timezone.utc)


def poll_confirm_url(config: ImapConfig, *, not_before: datetime, ignore: set[str]) -> str | None:
    try:
        return _latest_confirm_url(config, not_before, ignore)
    except (imaplib.IMAP4.abort, OSError) as exc:
        print(f"IMAP connection failed ({exc.__class__.__name__}); retrying...", flush=True)
        return None


def _latest_confirm_url(config: ImapConfig, not_before: datetime, ignore: set[str]) -> str | None:
    client = imaplib.IMAP4_SSL(config.host, config.port, timeout=60)
    try:
        try:
            client.login(config.user, config.password)
        except imaplib.IMAP4.abort:
            raise
        except imaplib.IMAP4.error as exc:
            raise PruneError("IMAP login failed; check PYPI_IMAP_USER and PYPI_IMAP_PASSWORD") from exc
        status, _ = client.select(config.mailbox, readonly=True)
        if status != "OK":
            raise PruneError(f"IMAP could not open mailbox {config.mailbox!r}")

        status, data = client.search(None, "FROM", "noreply@pypi.org")
        if status != "OK" or not data or not data[0]:
            return None

        newest: tuple[datetime, str] | None = None
        for message_id in data[0].split()[-30:]:
            status, fetched = client.fetch(message_id, "(BODY.PEEK[])")
            if status != "OK" or not fetched:
                continue
            raw = _first_message_bytes(fetched)
            if raw is None:
                continue
            sent_at = _message_datetime(raw)
            url = _confirm_url_from_message(raw)
            if sent_at is None or url is None or sent_at < not_before or url in ignore:
                continue
            if newest is None or sent_at > newest[0]:
                newest = (sent_at, url)
        if newest is None:
            return None
        return newest[1]
    finally:
        try:
            client.logout()
        except imaplib.IMAP4.error:
            pass


def _first_message_bytes(fetched: list) -> bytes | None:
    for item in fetched:
        if isinstance(item, tuple) and len(item) >= 2 and isinstance(item[1], bytes):
            return item[1]
    return None


class PypiSession:
    def __init__(self, origin: str, username: str, password: str, totp_secret: str, imap: ImapConfig | None):
        import requests

        self.origin = origin.rstrip("/")
        self.username = username
        self.password = password
        self.totp_secret = totp_secret
        self.imap = imap
        self.login_started = datetime.now(timezone.utc)
        self.session = requests.Session()
        self.session.headers["User-Agent"] = USER_AGENT

    def login(self) -> None:
        import pyotp

        login_url = f"{self.origin}/account/login/"
        page = self._get(login_url)
        csrf = csrf_for_input(page.text, "username")
        if not csrf:
            raise PruneError("Could not find the PyPI login form")

        # Record this before submitting TOTP: that request sends the device email.
        self.login_started = datetime.now(timezone.utc)
        response = self._post(
            login_url,
            {"csrf_token": csrf, "username": self.username, "password": self.password},
            referer=login_url,
        )
        if urlparse(response.url).path.rstrip("/") == "/account/login":
            raise PruneError("PyPI rejected PYPI_USERNAME or PYPI_PASSWORD")

        if "/account/two-factor" in urlparse(response.url).path:
            response = self._submit_totp(response.url, response.text, self._totp_code(pyotp))

        if _needs_device_confirm(response.url, response.text):
            self._confirm_new_device()

    def _totp_code(self, pyotp) -> str:
        try:
            return pyotp.TOTP(self.totp_secret).now()
        except (ValueError, TypeError) as exc:
            raise PruneError("PYPI_TOTP_SECRET is not a valid authenticator seed") from exc

    def _submit_totp(self, url: str, html: str, code: str):
        csrf = csrf_for_input(html, "totp_value")
        if not csrf:
            raise PruneError(
                "Could not find the PyPI authenticator-app form. "
                "This cleanup needs a TOTP device on the PyPI account, not only a security key."
            )
        response = self._post(url, {"csrf_token": csrf, "totp_value": code}, referer=url)
        if "/account/two-factor" in urlparse(response.url).path:
            raise PruneError("PyPI rejected the TOTP code from PYPI_TOTP_SECRET")
        return response

    def _confirm_new_device(self) -> None:
        if self.imap is None:
            raise PruneError(
                "PyPI does not recognize this runner IP and emailed a confirmation link. "
                "GitHub-hosted runners change IP every job, so set PYPI_IMAP_HOST, "
                "PYPI_IMAP_USER and PYPI_IMAP_PASSWORD to the inbox that receives "
                "mail from noreply@pypi.org. The message must land in PYPI_IMAP_MAILBOX "
                "(default INBOX)."
            )

        print("PyPI asked to confirm this device; reading the link from IMAP.", flush=True)
        # Mail servers can be a few seconds behind the runner clock.
        not_before = self.login_started - timedelta(seconds=30)
        deadline = time.monotonic() + 5 * 60
        ignored: set[str] = set()
        while True:
            confirm_url = poll_confirm_url(self.imap, not_before=not_before, ignore=ignored)
            if confirm_url:
                response = self._get(confirm_url)
                if self._device_confirmed(response):
                    return
                ignored.add(confirm_url)
            if time.monotonic() >= deadline:
                raise PruneError(
                    "PyPI asked to confirm this device, but no usable confirmation mail "
                    f"arrived in IMAP mailbox {self.imap.mailbox!r} within 5 minutes"
                )
            print("Waiting for the PyPI device-confirmation email...", flush=True)
            time.sleep(15)

    def _device_confirmed(self, response) -> bool:
        if _needs_device_confirm(response.url, response.text):
            return False
        if "invalid token" in response.text.lower():
            return False
        if "/account/login" in urlparse(response.url).path:
            return False
        return True

    def delete_release(self, package: str, version: str) -> None:
        page_url = f"{self.origin}/manage/project/{quote(package)}/release/{quote(version)}/"
        page = self._get(page_url)
        if "/account/login" in urlparse(page.url).path:
            raise PruneError("PyPI session is not logged in; cannot delete")

        csrf = csrf_for_input(page.text, "confirm_delete_version")
        if not csrf:
            raise PruneError(f"Release {version} has no delete form (missing, quarantined, or renamed)")

        response = self._post(
            page.url,
            {"csrf_token": csrf, "confirm_delete_version": version},
            referer=page.url,
        )
        final_path = urlparse(response.url).path.rstrip("/")
        if final_path.endswith("/releases"):
            return
        if final_path.endswith("/account/reauthenticate"):
            raise PruneError("PyPI asked to sign in again before deleting the release")
        raise PruneError(f"PyPI did not confirm deletion of {version}")

    def _get(self, url: str):
        import requests

        try:
            response = self.session.get(url, timeout=60)
        except requests.RequestException as exc:
            raise PruneError(f"GET failed: {exc.__class__.__name__}") from exc
        if response.status_code >= 400:
            raise PruneError(f"GET returned HTTP {response.status_code}")
        return response

    def _post(self, url: str, data: dict[str, str], *, referer: str):
        import requests

        try:
            response = self.session.post(
                url,
                data=data,
                headers={"Referer": referer, "Origin": self.origin},
                timeout=60,
            )
        except requests.RequestException as exc:
            raise PruneError(f"POST failed: {exc.__class__.__name__}") from exc
        if response.status_code >= 400:
            raise PruneError(f"POST returned HTTP {response.status_code}")
        return response


def fetch_project(package: str) -> dict:
    import requests

    response = requests.get(
        INDEX_JSON.format(package=package),
        headers={"User-Agent": USER_AGENT, "Accept": "application/json"},
        timeout=60,
    )
    if response.status_code == 404:
        raise PruneError(f"PyPI project {package!r} was not found")
    if response.status_code >= 400:
        raise PruneError(f"PyPI JSON API returned HTTP {response.status_code}")
    return response.json()


def describe(choice: Choice) -> str:
    uploaded = choice.release.uploaded_at.strftime("%Y-%m-%d")
    return f"{choice.release.version} ({choice.reason}, uploaded {uploaded})"


def prune(package: str, wheels_dir: Path, *, select_only: bool) -> None:
    wheels = local_wheels(wheels_dir)
    if not wheels:
        raise PruneError(f"No wheels found under {wheels_dir}")

    payload = fetch_project(package)
    remote_names = published_filenames(payload)
    local_names = {wheel.name for wheel in wheels}
    if local_names <= remote_names:
        print("Wheels are already on PyPI; not deleting anything.", flush=True)
        return

    choice = choose_victim(
        load_releases(payload),
        protect=wheel_versions(wheels),
        now=datetime.now(timezone.utc),
    )
    if choice is None:
        print("No older release is eligible for deletion.", flush=True)
        return

    print(f"Will delete {package} {describe(choice)}", flush=True)
    if select_only:
        return

    username, password, totp_secret = require_login_env()
    session = PypiSession(
        "https://pypi.org",
        username,
        password,
        totp_secret,
        load_imap_config(),
    )
    session.login()
    session.delete_release(package, choice.release.version)
    print(f"Deleted {package} {choice.release.version}", flush=True)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package", default="MaaFw")
    parser.add_argument("--wheels", type=Path, required=True)
    parser.add_argument(
        "--select-only",
        action="store_true",
        help="Print the release that would be deleted, without logging in",
    )
    args = parser.parse_args(argv)
    try:
        prune(args.package, args.wheels, select_only=args.select_only)
    except PruneError as exc:
        print(f"::error::{exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
