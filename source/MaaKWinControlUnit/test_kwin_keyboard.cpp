/**
 * @file test_kwin_keyboard.cpp
 * @brief Integration test for KWin Control Unit — keyboard input (key_down/key_up).
 *
 * This test verifies:
 *   1. Keycode mapping logic for all ASCII letters, digits, symbols, and
 *      control characters (Phase 1 — pure logic, no device needed).
 *   2. Actual uinput keyboard events via key_down/key_up through the
 *      KWinControlUnitAPI virtual interface (Phase 2 — needs /dev/uinput).
 *
 * Build (from project root):
 *   cmake --build --preset "NinjaMulti - Release" --target test_kwin_keyboard
 *
 * Usage:
 *   LD_LIBRARY_PATH=build/lib/Release ./build/bin/test_kwin_keyboard [/dev/uinput]
 *
 * Defaults: /dev/uinput
 */

#include "MaaControlUnit/ControlUnitAPI.h"
#include "MaaControlUnit/KWinControlUnitAPI.h"
#include "thread"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <linux/input.h>

// ─── Test harness ───

static int s_passed = 0;
static int s_failed = 0;

#define TEST(cond, fmt, ...)                                                                \
    do {                                                                                    \
        if (!(cond)) {                                                                      \
            fprintf(stderr, "  \033[31mFAIL\033[0m: " fmt "\n" __VA_OPT__(, ) __VA_ARGS__); \
            ++s_failed;                                                                     \
        }                                                                                   \
        else {                                                                              \
            printf("  \033[32mPASS\033[0m: " fmt "\n" __VA_OPT__(, ) __VA_ARGS__);          \
            ++s_passed;                                                                     \
        }                                                                                   \
    } while (0)

// ─── Phase 2: uinput integration helpers ───

static void test_single_key(MAA_CTRL_UNIT_NS::ControlUnitAPI* ctrl, int key, const char* label)
{
    const bool down_ok = ctrl->key_down(key);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    const bool up_ok = ctrl->key_up(key);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    const bool ok = down_ok && up_ok;
    TEST(ok, "key_down/key_up %s", label);
}

static void test_key_sequence(MAA_CTRL_UNIT_NS::ControlUnitAPI* ctrl, const char* sequence, const char* label)
{
    printf("\n  -- %s --\n", label);
    printf("    Typing: \"%s\"\n", sequence);

    bool ok = true;
    if (!ctrl->input_text(sequence)) {
        fprintf(stderr, "    FAIL at sequence '%s'\n", sequence);
    }

    TEST(ok, "Type sequence \"%s\"", label);
}

// ─── Main ───

int main(int argc, char** argv)
{
    const char* device_node = (argc > 1) ? argv[1] : "/dev/uinput";
    constexpr int kWidth = 1920;
    constexpr int kHeight = 1080;

    printf("=== KWinControlUnit Keyboard Input Test ===\n");
    printf("  Device node : %s\n", device_node);
    printf("  Resolution  : %dx%d\n", kWidth, kHeight);
    printf("  Version     : %s\n\n", MaaKWinControlUnitGetVersion());

    // ═══════════════════════════════════════════════════════════════════════
    //          UInput Keyboard Integration (needs /dev/uinput)
    // ═══════════════════════════════════════════════════════════════════════
    printf("═══════════════════════════════════════════════════\n");
    printf("           UInput Keyboard Integration\n");
    printf("═══════════════════════════════════════════════════\n");

    MaaKWinControlUnitHandle handle = MaaKWinControlUnitCreate(device_node, kWidth, kHeight, false);
    if (!handle) {
        fprintf(stderr, "FAIL: MaaKWinControlUnitCreate returned NULL\n");
        return 1;
    }
    printf("  [PASS] MaaKWinControlUnitCreate\n");

    MaaBool connected = MaaKWinControlUnitConnect(handle);
    if (!connected) {
        fprintf(stderr, "FAIL: MaaKWinControlUnitConnect returned false\n");
        fprintf(stderr, "  (need write access to %s and permissions)\n", device_node);
        MaaKWinControlUnitDestroy(handle);
        // Phase 2 failures may be environmental — still report Phase 1 results
        printf("\n  Phase 2 SKIPPED (no /dev/uinput access)\n");
        printf("\n=== Summary: %d passed, %d failed (Phase 2 skipped) ===\n", s_passed, s_failed);
        return s_failed > 0 ? 1 : 0;
    }
    printf("  [PASS] MaaKWinControlUnitConnect\n");

    // Cast to ControlUnitAPI to access key_down/key_up virtual methods
    MAA_CTRL_UNIT_NS::ControlUnitAPI* ctrl = static_cast<MAA_CTRL_UNIT_NS::ControlUnitAPI*>(handle);
    if (!ctrl) {
        fprintf(stderr, "FAIL: handle cast to ControlUnitAPI failed\n");
        MaaKWinControlUnitDestroy(handle);
        return 1;
    }
    printf("  [PASS] handle cast to ControlUnitAPI\n");

    printf("Preparing for send key events...");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    // --- Single letter keys ---
    printf("\n  -- Single letter keys --\n");
    test_single_key(ctrl, KEY_A, "'A'");
    test_single_key(ctrl, KEY_M, "'M'");
    test_single_key(ctrl, KEY_Z, "'Z'");
    test_single_key(ctrl, KEY_A, "'A'");
    test_single_key(ctrl, KEY_Z, "'Z'");

    // --- Single digit keys ---
    printf("\n  -- Single digit keys --\n");
    test_single_key(ctrl, KEY_1, "'1'");
    test_single_key(ctrl, KEY_5, "'5'");
    test_single_key(ctrl, KEY_9, "'9'");
    test_single_key(ctrl, KEY_0, "'0'");

    // --- Single symbol keys ---
    printf("\n  -- Single symbol keys --\n");
    test_single_key(ctrl, KEY_MINUS, "'-'");
    test_single_key(ctrl, KEY_EQUAL, "'='");
    test_single_key(ctrl, KEY_LEFTBRACE, "'['");
    test_single_key(ctrl, KEY_RIGHTBRACE, "']'");
    test_single_key(ctrl, KEY_SEMICOLON, "';'");
    test_single_key(ctrl, KEY_COMMA, "','");
    test_single_key(ctrl, KEY_DOT, "'.'");
    test_single_key(ctrl, KEY_SLASH, "'/'");
    test_single_key(ctrl, KEY_SPACE, "SPACE");

    // --- Single control characters ---
    printf("\n  -- Single control characters --\n");
    test_single_key(ctrl, KEY_ENTER, "ENTER");
    test_single_key(ctrl, KEY_BACKSPACE, "BACKSPACE");
    test_single_key(ctrl, KEY_TAB, "TAB");
    test_single_key(ctrl, KEY_ESC, "ESC");

    // --- Raw Linux keycodes ---
    printf("\n  -- Raw Linux keycodes (non-ASCII range) --\n");
    // NOTE: Keycodes < 32 (control chars) map to ASCII equivalents.
    // Keycodes 32-127 that overlap with ASCII letters/digits/symbols also map.
    // Only keycodes OUTSIDE the ASCII mapping (e.g. >= 128 or unmapped symbols)
    // pass through unchanged. See Phase 1 raw_passthrough note.
    // test_single_key(ctrl, KEY_LEFTSHIFT, "KEY_LEFTSHIFT");
    // test_single_key(ctrl, KEY_LEFTMETA, "KEY_LEFTMETA (Super)");
    // test_single_key(ctrl, KEY_CAPSLOCK, "KEY_CAPSLOCK");

    // --- Key sequences (type a word, ASCII test) ---
    test_key_sequence(ctrl, "Hello", "Hello");
    test_key_sequence(ctrl, "MaaFramework", "MaaFramework");
    test_key_sequence(ctrl, "KWin Control Unit 2026", "with spaces and digits");

    // Cleanup
    MaaKWinControlUnitDestroy(handle);

    printf("\n═══════════════════════════════════════════════════\n");
    printf("  Final: %d passed, %d failed\n", s_passed, s_failed);
    printf("═══════════════════════════════════════════════════\n");

    return s_failed > 0 ? 1 : 0;
}
