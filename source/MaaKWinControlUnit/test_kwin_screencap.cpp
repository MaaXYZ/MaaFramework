/**
 * @file test_kwin_screencap.c
 * @brief Integration test for KWin Control Unit — controller connect + screencap.
 *
 * Build (from project root):
 *   cmake --build --preset "NinjaMulti - Release" --target test_kwin_screencap
 *
 * Usage:
 *   LD_LIBRARY_PATH=build/lib/Release ./build/bin/test_kwin_screencap [/dev/uinput] [width] [height]
 *
 * Defaults: /dev/uinput  1920 1080
 */

#include "MaaControlUnit/KWinControlUnitAPI.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{
    const char* device_node = (argc > 1) ? argv[1] : "/dev/uinput";
    int width = (argc > 2) ? atoi(argv[2]) : 1920;
    int height = (argc > 3) ? atoi(argv[3]) : 1080;

    printf("=== KWinControlUnit Integration Test ===\n");
    printf("Device node : %s\n", device_node);
    printf("Resolution  : %dx%d\n", width, height);
    printf("Version     : %s\n\n", MaaKWinControlUnitGetVersion());

    /* 1. Create */
    MaaKWinControlUnitHandle handle = MaaKWinControlUnitCreate(device_node, width, height, false);
    if (!handle) {
        fprintf(stderr, "FAIL: MaaKWinControlUnitCreate returned NULL\n");
        return 1;
    }
    printf("[PASS] MaaKWinControlUnitCreate\n");

    /* 2. Connect (opens /dev/uinput + establishes PipeWire screencast session) */
    MaaBool connected = MaaKWinControlUnitConnect(handle);
    if (!connected) {
        fprintf(stderr, "FAIL: MaaKWinControlUnitConnect returned false\n");
        MaaKWinControlUnitDestroy(handle);
        return 1;
    }
    printf("[PASS] MaaKWinControlUnitConnect\n");

    /* 3. Test screencap (capture one frame via PipeWire) */
    MaaBool screencap_ok = MaaKWinControlUnitTestScreencap(handle);
    if (!screencap_ok) {
        fprintf(stderr, "FAIL: MaaKWinControlUnitTestScreencap returned false\n");
        MaaKWinControlUnitDestroy(handle);
        return 1;
    }
    printf("[PASS] MaaKWinControlUnitTestScreencap\n");

    /* 4. Cleanup */
    MaaKWinControlUnitDestroy(handle);
    printf("\n=== All tests PASSED ===\n");
    return 0;
}
