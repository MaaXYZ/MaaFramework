"""Exercise log rotation and background maintenance through the native library."""

import gzip
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest


MIB = 1024 * 1024
DAY = 24 * 60 * 60


class LogMaintenanceTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix="maafw-logs-")
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name) / "日志"
        self.root.mkdir()

    def write_file(self, name, content=b"diagnostic evidence\n", age_days=0, size=None):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("wb") as stream:
            stream.write(content)
            if size is not None:
                stream.truncate(size)
        modified = time.time() - age_days * DAY
        os.utime(path, (modified, modified))
        return path

    def maintain(self, prelude=""):
        # Process exit joins the native maintenance worker, avoiding sleeps and
        # changes to the parent test process's global logging configuration.
        result = subprocess.run(
            [
                sys.executable,
                "-c",
                prelude + "import sys; from maa.tasker import Tasker; "
                "assert Tasker.set_log_dir(sys.argv[1])",
                str(self.root),
            ],
            capture_output=True,
            text=True,
            timeout=60,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_gzip_round_trip_preserves_age_and_active_log(self):
        content = ("识别日志\x00\r\n".encode() + bytes(range(256))) * 1024
        backup = self.write_file("maafw.bak.roundtrip.log", content, age_days=2)
        modified = backup.stat().st_mtime
        active = self.write_file("maafw.log", b"active log\n", age_days=10)
        unrelated = self.write_file("other.log", age_days=10)

        self.maintain()

        archive = backup.with_suffix(".log.gz")
        with gzip.open(archive, "rb") as stream:
            self.assertEqual(stream.read(), content)
        self.assertEqual(archive.read_bytes()[8], 4)  # gzip's fastest-compression flag
        self.assertAlmostEqual(archive.stat().st_mtime, modified, delta=2)
        self.assertFalse(backup.exists())
        self.assertTrue(active.read_bytes().startswith(b"active log\n"))
        self.assertTrue(unrelated.exists())
        self.assertFalse(list(self.root.glob("*.tmp")))

        before = archive.read_bytes()
        self.maintain()
        self.assertEqual(archive.read_bytes(), before)

    def test_retention_and_images(self):
        expired = [
            self.write_file("maafw.bak.old.log", age_days=8),
            self.write_file("maafw.bak.old.log.gz", age_days=8),
            self.write_file("maafw.bak.old.log.gz.interrupted.tmp", age_days=8),
            self.write_file("vision/old.jpg", age_days=4),
            self.write_file("on_error/old.png", age_days=8),
        ]
        retained = [
            self.write_file("vision/new.jpg", b"jpeg bytes", age_days=2),
            self.write_file("on_error/recent.png", b"png bytes", age_days=6),
            self.write_file("resource/template.png", b"user image", age_days=10),
        ]
        contents = {path: path.read_bytes() for path in retained}

        self.maintain()

        for path in expired:
            self.assertFalse(path.exists(), str(path))
        for path, content in contents.items():
            self.assertEqual(path.read_bytes(), content)

    def test_budget_prefers_vision_and_then_oldest_logs(self):
        vision = self.write_file("vision/recent.jpg", size=512 * MIB)
        older = self.write_file("maafw.bak.older.log.gz", age_days=2, size=600 * MIB)
        newer = self.write_file("maafw.bak.newer.log.gz", age_days=1, size=400 * MIB)
        failure = self.write_file("on_error/failure.png", age_days=3, size=50 * MIB)

        self.maintain()

        self.assertFalse(vision.exists())
        self.assertFalse(older.exists())
        self.assertTrue(newer.exists())
        self.assertTrue(failure.exists())

    def test_failed_compression_preserves_original(self):
        original = self.write_file("maafw.bak.blocked.log", b"must survive\n")
        original.with_suffix(".log.gz").mkdir()

        self.maintain()

        self.assertEqual(original.read_bytes(), b"must survive\n")
        self.assertFalse(list(self.root.glob("*.tmp")))

    def test_rotation_threshold(self):
        active = self.write_file("maafw.log", b"before rotation\n", size=127 * MIB)
        self.maintain()
        self.assertFalse(list(self.root.glob("maafw.bak.*")))

        with active.open("r+b") as stream:
            stream.truncate(128 * MIB)
        self.maintain()

        archives = list(self.root.glob("maafw.bak.*.log.gz"))
        self.assertEqual(len(archives), 1)
        size = 0
        with gzip.open(archives[0], "rb") as stream:
            self.assertEqual(stream.read(16), b"before rotation\n")
            size += 16
            while chunk := stream.read(MIB):
                size += len(chunk)
        self.assertEqual(size, 128 * MIB)
        self.assertLess(active.stat().st_size, MIB)

    @unittest.skipIf(os.name == "nt", "requires POSIX file-size limits")
    def test_compression_write_failure_keeps_source(self):
        content = os.urandom(128 * 1024)
        original = self.write_file("maafw.bak.write-failure.log", content)

        self.maintain(
            "import resource, signal; "
            "signal.signal(signal.SIGXFSZ, signal.SIG_IGN); "
            "resource.setrlimit(resource.RLIMIT_FSIZE, (4096, 4096)); "
        )

        self.assertEqual(original.read_bytes(), content)
        self.assertFalse(original.with_suffix(".log.gz").exists())
        self.assertFalse(list(self.root.glob("*.tmp")))

    @unittest.skipIf(os.name == "nt", "requires POSIX file-size limits")
    def test_rotation_write_failure_keeps_active_log(self):
        active = self.write_file("maafw.log", b"must survive\n", size=128 * MIB)

        self.maintain(
            "import resource, signal; "
            "signal.signal(signal.SIGXFSZ, signal.SIG_IGN); "
            "resource.setrlimit(resource.RLIMIT_FSIZE, (4096, 4096)); "
        )

        self.assertEqual(active.stat().st_size, 128 * MIB)
        with active.open("rb") as stream:
            self.assertEqual(stream.read(13), b"must survive\n")
        self.assertFalse(list(self.root.glob("maafw.bak.*")))

    @unittest.skipIf(os.name == "nt", "symlink creation requires privileges on Windows")
    def test_symlinks_are_not_compressed_or_cleaned(self):
        target = Path(self.directory.name) / "outside.log"
        target.write_bytes(b"outside evidence")
        modified = time.time() - 10 * DAY
        os.utime(target, (modified, modified))
        link = self.root / "maafw.bak.symlink.log"
        link.symlink_to(target)
        (self.root / "vision").symlink_to(target.parent, target_is_directory=True)

        self.maintain()

        self.assertTrue(link.is_symlink())
        self.assertEqual(target.read_bytes(), b"outside evidence")


if __name__ == "__main__":
    unittest.main()
