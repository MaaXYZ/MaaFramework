"""Structured iOS RemotePairing credentials; persistence is provided by Toolkit."""

import ctypes
from dataclasses import dataclass, field

from .define import MaaIOSWiFiPairing


@dataclass
class IOSWiFiPairing:
    udid: str
    host_identifier: str
    peer_identifier: str
    host_private_key: bytes = field(repr=False)
    host_public_key: bytes
    peer_public_key: bytes
    peer_alt_irk: bytes = field(default=b"", repr=False)

    def _native(self) -> MaaIOSWiFiPairing:
        if not self.udid or not self.host_identifier or "\0" in self.udid or "\0" in self.host_identifier or "\0" in self.peer_identifier:
            raise ValueError("Pairing requires device and host identities without NUL")
        if any(len(key) != 32 for key in (self.host_private_key, self.host_public_key, self.peer_public_key)):
            raise ValueError("Pairing signing keys must contain 32 bytes")
        if not 0 < len(self.peer_identifier.encode()) <= 4096 or len(self.peer_alt_irk) not in (0, 16):
            raise ValueError("Invalid pairing peer identity or IRK length")
        result = MaaIOSWiFiPairing()
        result.udid = self.udid.encode()
        result.host_identifier = self.host_identifier.encode()
        result.peer_identifier = self.peer_identifier.encode()
        for name in ("host_private_key", "host_public_key", "peer_public_key"):
            getattr(result, name)[:] = getattr(self, name)
        if self.peer_alt_irk:
            result.peer_alt_irk[:] = self.peer_alt_irk
        result.has_peer_alt_irk = bool(self.peer_alt_irk)
        return result

    @classmethod
    def _from_native(cls, value: MaaIOSWiFiPairing) -> "IOSWiFiPairing":
        return cls(
            udid=value.udid.decode(),
            host_identifier=value.host_identifier.decode(),
            peer_identifier=value.peer_identifier.decode(),
            host_private_key=ctypes.string_at(value.host_private_key, 32),
            host_public_key=ctypes.string_at(value.host_public_key, 32),
            peer_public_key=ctypes.string_at(value.peer_public_key, 32),
            peer_alt_irk=ctypes.string_at(value.peer_alt_irk, 16) if value.has_peer_alt_irk else b"",
        )
