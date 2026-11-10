"""Discover unit tests and fail on destructor or background-thread exceptions."""

import argparse
import gc
from pathlib import Path
import sys
import threading
import traceback
import unittest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pattern", default="test_*.py")
    args = parser.parse_args()
    unhandled = []
    original_unraisable = sys.unraisablehook
    original_thread = threading.excepthook

    def unraisable_error(event):
        unhandled.append((event.exc_type, event.exc_value, event.exc_traceback))
        original_unraisable(event)

    def thread_error(event):
        unhandled.append((event.exc_type, event.exc_value, event.exc_traceback))
        original_thread(event)

    sys.unraisablehook = unraisable_error
    threading.excepthook = thread_error
    try:
        directory = Path(__file__).resolve().parent / "python"
        suite = unittest.defaultTestLoader.discover(str(directory), pattern=args.pattern)
        if suite.countTestCases() == 0:
            print("No unit tests discovered", file=sys.stderr)
            return 1
        result = unittest.TextTestRunner(verbosity=2).run(suite)
        gc.collect()
        if unhandled:
            print(
                f"\nFAIL: {len(unhandled)} unhandled destructor/thread exception(s)",
                file=sys.stderr,
            )
            for exception in unhandled:
                traceback.print_exception(*exception)
        return 0 if result.wasSuccessful() and not unhandled else 1
    finally:
        sys.unraisablehook = original_unraisable
        threading.excepthook = original_thread


if __name__ == "__main__":
    sys.exit(main())
