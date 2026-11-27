# Linux resource cleanup regression tests

These tests run without a desktop session, a PipeWire server, or input devices.
They are also registered with CTest in a full build configured with
`-DBUILD_LINUX_TEST=ON`.

The standalone builds require CMake 3.28 or newer, a C++20 compiler, and Ninja
(or another CMake generator). On Debian/Ubuntu, the other development packages
are `pkg-config`, `dbus`, `libdbus-1-dev`, `libglib2.0-dev`, and
`libopencv-core-dev`. Run the following commands from the repository root.

## Portal

```sh
cmake -S test/linux_test/portal_cleanup -B build/portal-cleanup -G Ninja
cmake --build build/portal-cleanup
ctest --test-dir build/portal-cleanup --output-on-failure
```

The target compiles the production Portal implementation and C API against real
libdbus. Only logging is substituted. GTestDBus creates its own private bus and
the test runs a simulated ScreenCast service on it. No existing session bus or
desktop authorization is used. Timeout cases exercise the production 30-second
deadline; allow approximately two minutes for the suite. Individual cases can
be selected with `build/portal-cleanup/portal_fd_test -p /portal/failure/start-cancel`.

The tests check request/session closure, FD ownership and counts, failed opens,
retries, destruction, and asynchronous signal handling. They do not exercise a
real compositor or PipeWire stream. Exported sessions are deliberately retained
for the existing caller ownership contract; passing this suite does not establish
that their full lifecycle is resolved.

## Controller manager

```sh
git submodule update --init source/MaaUtils
cmake -S test/linux_test/manager_cleanup -B build/manager-cleanup -G Ninja
cmake --build build/manager-cleanup
ctest --test-dir build/manager-cleanup --output-on-failure
```

The target compiles the production manager, configuration, and common control
unit API. Hardware backends are substituted with fault-injectable objects that
hold real file descriptors. This verifies immediate release after failure,
retry/reconnect behavior, and destruction order. It does not verify the real
PipeWire backend's ownership of the configured `pw_socket_fd`.

For an existing MaaUtils checkout, pass `-DMAA_UTILS_ROOT=/path/to/MaaUtils`.
To run either standalone target with ASan/UBSan, configure a separate build with
`-DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -g'`
and `-DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'`, then build and run
CTest as above.

## ext-image-copy-capture session benchmark

```sh
cmake -S . -B build -G "Ninja Multi-Config" -DBUILD_LINUX_TEST=ON
cmake --build build --config Debug --target ext_image_screencap_bench
WAYLAND_DISPLAY=wayland-0 build/bin/Debug/ext_image_screencap_bench [wayland-socket] [iterations]
```

`ext_image_screencap_bench` is a manual benchmark, not a CTest case: it needs a
live compositor that implements `ext-image-capture-source-v1` and
`ext-image-copy-capture-v1` (e.g. wlroots/sway >= 0.19). It compares the two
session strategies behind the `ExtImage` screencap method:

- **A, fresh session per capture** — what `ExtImageScreencap` does. Every frame
  is the session's first frame, which the compositor must copy immediately, so
  captures return in single-digit milliseconds even on an idle screen.
- **B, persistent session** — reused for comparison. After the first frame the
  compositor may wait indefinitely for content changes; the benchmark shows
  per-capture latency degrading to the compositor's damage cadence, plus a
  bounded-timeout check on a static screen.

The tool intentionally links the protocol code directly instead of the
`MaaLinuxControlUnit` library so the strategy flows can diverge from the
production code path.
