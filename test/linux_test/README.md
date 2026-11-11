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
