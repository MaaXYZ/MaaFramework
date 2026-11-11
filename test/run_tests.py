"""Run native-backed Python unit tests and integration suites in isolated processes."""

import argparse
import os
from pathlib import Path
import signal
import subprocess
import sys
import time


ROOT = Path(__file__).resolve().parent.parent
SUITES = {
    "unit": ("test/run_unit_tests.py",),
    "binding": ("test/python/binding_test.py",),
    "pipeline": ("test/python/pipeline_test.py",),
    "agent": ("test/agent/agent_main_test.py",),
    "agent-tcp": ("test/agent/agent_tcp_main_test.py",),
}


def stop_suite(process):
    if os.name == "nt":
        subprocess.run(
            ["taskkill", "/PID", str(process.pid), "/T", "/F"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
            timeout=15,
        )
    else:
        try:
            os.killpg(process.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
    if process.poll() is None:
        process.kill()
    process.wait(timeout=15)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binding_dir", type=Path)
    parser.add_argument("install_dir", type=Path)
    parser.add_argument(
        "--suite",
        action="append",
        choices=SUITES,
        help="Select suites; repeat to run several (default: all)",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=600,
        help="Timeout in seconds per suite (default: 600)",
    )
    args = parser.parse_args()
    binding_dir = args.binding_dir.resolve()
    install_dir = args.install_dir.resolve()
    if not (binding_dir / "maa").is_dir():
        parser.error(f"Python binding package not found: {binding_dir / 'maa'}")
    if not (install_dir / "bin").is_dir():
        parser.error(f"Native binary directory not found: {install_dir / 'bin'}")
    if args.timeout <= 0:
        parser.error("--timeout must be positive")

    environment = os.environ.copy()
    environment["MAAFW_BINARY_PATH"] = str(install_dir / "bin")
    environment["PYTHONPATH"] = os.pathsep.join(filter(None, (str(binding_dir), environment.get("PYTHONPATH"))))
    environment["PATH"] = str(install_dir / "bin") + os.pathsep + environment.get("PATH", "")
    environment["PYTHONUTF8"] = "1"
    results = []
    for name in dict.fromkeys(args.suite or SUITES):
        command = [sys.executable, "-X", "utf8", *SUITES[name]]
        if name != "unit":
            command.extend((str(binding_dir), str(install_dir)))
        print(f"\n=== {name} ===", flush=True)
        started = time.monotonic()
        process = subprocess.Popen(command, cwd=ROOT, env=environment, start_new_session=os.name != "nt")
        try:
            returncode = process.wait(timeout=args.timeout)
            outcome = "PASS" if returncode == 0 else f"FAIL (exit {returncode})"
        except subprocess.TimeoutExpired:
            stop_suite(process)
            outcome = f"TIMEOUT ({args.timeout:g}s)"
        except KeyboardInterrupt:
            stop_suite(process)
            raise
        results.append((name, outcome, time.monotonic() - started))

    print("\nSuite results:", flush=True)
    for name, outcome, elapsed in results:
        print(f"  {name}: {outcome} [{elapsed:.1f}s]", flush=True)
    return 0 if all(outcome == "PASS" for _, outcome, _ in results) else 1


if __name__ == "__main__":
    sys.exit(main())
