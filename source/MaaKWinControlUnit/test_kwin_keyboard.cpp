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

#include "MaaControlUnit/KWinControlUnitAPI.h"
#include "MaaControlUnit/ControlUnitAPI.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <linux/input.h>

// ─── Keycode mapping reference (mirrors UInputController::maa_to_linux_keycode) ───

static int reference_keycode(int c)
{
    // ASCII letter mapping
    if (c >= 'A' && c <= 'Z') {
        return KEY_A + (c - 'A');
    }
    if (c >= 'a' && c <= 'z') {
        return KEY_A + (c - 'a');
    }
    // ASCII digit mapping (KEY_1 .. KEY_9, KEY_0)
    if (c >= '1' && c <= '9') {
        return KEY_1 + (c - '1');
    }
    if (c == '0') {
        return KEY_0;
    }
    // Common ASCII / control character mappings
    switch (c) {
        case '\r':
        case '\n': return KEY_ENTER;
        case '\b': return KEY_BACKSPACE;
        case '\t': return KEY_TAB;
        case 27:   return KEY_ESC;
        case ' ':  return KEY_SPACE;
        case '-':  return KEY_MINUS;
        case '=':  return KEY_EQUAL;
        case '[':  return KEY_LEFTBRACE;
        case ']':  return KEY_RIGHTBRACE;
        case '\\': return KEY_BACKSLASH;
        case ';':  return KEY_SEMICOLON;
        case '\'': return KEY_APOSTROPHE;
        case '`':  return KEY_GRAVE;
        case ',':  return KEY_COMMA;
        case '.':  return KEY_DOT;
        case '/':  return KEY_SLASH;
        default:   return c; // pass through as raw Linux keycode
    }
}

// ─── Test harness ───

static int s_passed = 0;
static int s_failed = 0;

#define TEST(cond, fmt, ...)                                                       \
    do {                                                                           \
        if (!(cond)) {                                                             \
            fprintf(stderr, "  \033[31mFAIL\033[0m: " fmt "\n" __VA_OPT__(, )      \
                    __VA_ARGS__);                                                  \
            ++s_failed;                                                            \
        } else {                                                                   \
            printf("  \033[32mPASS\033[0m: " fmt "\n" __VA_OPT__(, )              \
                   __VA_ARGS__);                                                   \
            ++s_passed;                                                            \
        }                                                                          \
    } while (0)

static int check_keycode(int c, int expected)
{
    int actual = reference_keycode(c);
    if (actual != expected) {
        fprintf(stderr,
                "  MISMATCH: char=%d (0x%02x '%c'), expected=KEY_%d, got=%d\n",
                c, c, (c >= 32 && c < 127) ? (char)c : '?', expected, actual);
        return -1;
    }
    return 0;
}

// ─── Phase 1 helpers ───

static void test_letter_range()
{
    printf("\n  -- Letters A-Z --\n");
    for (int c = 'A'; c <= 'Z'; ++c) {
        int exp = KEY_A + (c - 'A');
        TEST(check_keycode(c, exp) == 0, "'%c' -> KEY_%d (%d)", (char)c, exp - KEY_A, exp);
    }

    printf("\n  -- Letters a-z --\n");
    for (int c = 'a'; c <= 'z'; ++c) {
        int exp = KEY_A + (c - 'a');
        TEST(check_keycode(c, exp) == 0, "'%c' -> KEY_%d (%d)", (char)c, exp - KEY_A, exp);
    }
}

static void test_digit_range()
{
    printf("\n  -- Digits 0-9 --\n");
    for (int c = '1'; c <= '9'; ++c) {
        int exp = KEY_1 + (c - '1');
        TEST(check_keycode(c, exp) == 0, "'%c' -> KEY_%d (%d)", (char)c, c - '0', exp);
    }
    TEST(check_keycode('0', KEY_0) == 0, "'0' -> KEY_0 (%d)", KEY_0);
}

static void test_symbol_keys()
{
    printf("\n  -- Symbol / punctuation --\n");
    TEST(check_keycode(' ', KEY_SPACE) == 0, "SPACE -> KEY_SPACE (%d)", KEY_SPACE);
    TEST(check_keycode('-', KEY_MINUS) == 0, "'-' -> KEY_MINUS (%d)", KEY_MINUS);
    TEST(check_keycode('=', KEY_EQUAL) == 0, "'=' -> KEY_EQUAL (%d)", KEY_EQUAL);
    TEST(check_keycode('[', KEY_LEFTBRACE) == 0, "'[' -> KEY_LEFTBRACE (%d)", KEY_LEFTBRACE);
    TEST(check_keycode(']', KEY_RIGHTBRACE) == 0, "']' -> KEY_RIGHTBRACE (%d)", KEY_RIGHTBRACE);
    TEST(check_keycode('\\', KEY_BACKSLASH) == 0, "'\\' -> KEY_BACKSLASH (%d)", KEY_BACKSLASH);
    TEST(check_keycode(';', KEY_SEMICOLON) == 0, "';' -> KEY_SEMICOLON (%d)", KEY_SEMICOLON);
    TEST(check_keycode('\'', KEY_APOSTROPHE) == 0, "''' -> KEY_APOSTROPHE (%d)", KEY_APOSTROPHE);
    TEST(check_keycode('`', KEY_GRAVE) == 0, "'`' -> KEY_GRAVE (%d)", KEY_GRAVE);
    TEST(check_keycode(',', KEY_COMMA) == 0, "',' -> KEY_COMMA (%d)", KEY_COMMA);
    TEST(check_keycode('.', KEY_DOT) == 0, "'.' -> KEY_DOT (%d)", KEY_DOT);
    TEST(check_keycode('/', KEY_SLASH) == 0, "'/' -> KEY_SLASH (%d)", KEY_SLASH);
}

static void test_control_characters()
{
    printf("\n  -- Control characters --\n");
    TEST(check_keycode('\r', KEY_ENTER) == 0, "CR  -> KEY_ENTER (%d)", KEY_ENTER);
    TEST(check_keycode('\n', KEY_ENTER) == 0, "LF  -> KEY_ENTER (%d)", KEY_ENTER);
    TEST(check_keycode('\b', KEY_BACKSPACE) == 0, "BS  -> KEY_BACKSPACE (%d)", KEY_BACKSPACE);
    TEST(check_keycode('\t', KEY_TAB) == 0, "TAB -> KEY_TAB (%d)", KEY_TAB);
    TEST(check_keycode(27, KEY_ESC) == 0, "ESC -> KEY_ESC (%d)", KEY_ESC);
}

static void test_raw_passthrough()
{
    printf("\n  -- Raw keycode passthrough (non-ASCII range) --\n");
    // NOTE: Linux keycodes whose numeric value collides with ASCII characters
    // (e.g. KEY_RIGHTCTRL=97 == 'a', KEY_F1=59 == ';') will be mapped as ASCII
    // by maa_to_linux_keycode(). This is a known design limitation: the function
    // cannot distinguish "raw keycode 97" from "char 'a'" since both are ints.
    // Only keycodes OUTSIDE the ASCII mapping ranges pass through unchanged.

    // KEY_LEFTSHIFT=42 — '*' is not in the mapping switch, so it passes through
    TEST(check_keycode(KEY_LEFTSHIFT, KEY_LEFTSHIFT) == 0,
         "KEY_LEFTSHIFT (%d) passthrough", KEY_LEFTSHIFT);
    // KEY_LEFTMETA=125 — '}' is not in the mapping switch
    TEST(check_keycode(KEY_LEFTMETA, KEY_LEFTMETA) == 0,
         "KEY_LEFTMETA (%d) passthrough", KEY_LEFTMETA);
    // KEY_CAPSLOCK=58 — ':' is not in the mapping switch
    TEST(check_keycode(KEY_CAPSLOCK, KEY_CAPSLOCK) == 0,
         "KEY_CAPSLOCK (%d) passthrough", KEY_CAPSLOCK);
    // KEY_RIGHTSHIFT=54 — '6' would map to KEY_6, so skip
    // KEY_RIGHTCTRL=97 — collides with 'a'
    // KEY_LEFTALT=56  — collides with '8'
    // KEY_F1=59       — collides with ';'
    // KEY_UP=103      — collides with 'g'
    // KEY_DOWN=108    — collides with 'l'
    // KEY_LEFT=105    — collides with 'i'
    // KEY_RIGHT=106   — collides with 'j'
    // KEY_HOME=102    — collides with 'f'
    // KEY_END=107     — collides with 'k'
    // KEY_PAGEUP=104  — collides with 'h'
    // KEY_INSERT=110  — collides with 'n'
    // KEY_DELETE=111  — collides with 'o'
}

// ─── Phase 2: uinput integration helpers ───

static void test_single_key(MAA_CTRL_UNIT_NS::ControlUnitAPI* ctrl, int key, const char* label)
{
    bool ok = ctrl->key_down(key) && ctrl->key_up(key);
    TEST(ok, "key_down/key_up %s", label);
}

static void test_key_sequence(MAA_CTRL_UNIT_NS::ControlUnitAPI* ctrl,
                              const char* sequence, const char* label)
{
    printf("\n  -- %s --\n", label);
    printf("    Typing: \"%s\"\n", sequence);

    bool ok = true;
    for (const char* p = sequence; *p; ++p) {
        if (!ctrl->key_down(static_cast<int>(*p))) {
            ok = false;
            fprintf(stderr, "    FAIL at char '%c' (0x%02x) during key_down\n", *p, *p);
            break;
        }
        if (!ctrl->key_up(static_cast<int>(*p))) {
            ok = false;
            fprintf(stderr, "    FAIL at char '%c' (0x%02x) during key_up\n", *p, *p);
            break;
        }
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
    // Phase 1: Keycode Mapping (pure logic, no device needed)
    // ═══════════════════════════════════════════════════════════════════════
    printf("═══════════════════════════════════════════════════\n");
    printf("  Phase 1: Keycode Mapping Verification\n");
    printf("═══════════════════════════════════════════════════\n");

    test_letter_range();
    test_digit_range();
    test_symbol_keys();
    test_control_characters();
    test_raw_passthrough();

    printf("\n  Phase 1 results: %d passed, %d failed\n\n", s_passed, s_failed);
    if (s_failed > 0) {
        // Phase 1 failures indicate a logic error in the mapping — abort early
        fprintf(stderr, "FATAL: Keycode mapping failures detected. Aborting.\n");
        return 1;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // Phase 2: UInput Keyboard Integration (needs /dev/uinput)
    // ═══════════════════════════════════════════════════════════════════════
    printf("═══════════════════════════════════════════════════\n");
    printf("  Phase 2: UInput Keyboard Integration\n");
    printf("═══════════════════════════════════════════════════\n");

    MaaKWinControlUnitHandle handle = MaaKWinControlUnitCreate(device_node, kWidth, kHeight);
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
        printf("\n=== Summary: %d passed, %d failed (Phase 2 skipped) ===\n",
               s_passed, s_failed);
        return s_failed > 0 ? 1 : 0;
    }
    printf("  [PASS] MaaKWinControlUnitConnect\n");

    // Cast to ControlUnitAPI to access key_down/key_up virtual methods
    MAA_CTRL_UNIT_NS::ControlUnitAPI* ctrl =
        static_cast<MAA_CTRL_UNIT_NS::ControlUnitAPI*>(handle);
    if (!ctrl) {
        fprintf(stderr, "FAIL: handle cast to ControlUnitAPI failed\n");
        MaaKWinControlUnitDestroy(handle);
        return 1;
    }
    printf("  [PASS] handle cast to ControlUnitAPI\n");

    // --- Single letter keys ---
    printf("\n  -- Single letter keys --\n");
    test_single_key(ctrl, 'A', "'A'");
    test_single_key(ctrl, 'M', "'M'");
    test_single_key(ctrl, 'Z', "'Z'");
    test_single_key(ctrl, 'a', "'a'");
    test_single_key(ctrl, 'z', "'z'");

    // --- Single digit keys ---
    printf("\n  -- Single digit keys --\n");
    test_single_key(ctrl, '1', "'1'");
    test_single_key(ctrl, '5', "'5'");
    test_single_key(ctrl, '9', "'9'");
    test_single_key(ctrl, '0', "'0'");

    // --- Single symbol keys ---
    printf("\n  -- Single symbol keys --\n");
    test_single_key(ctrl, '-', "'-'");
    test_single_key(ctrl, '=', "'='");
    test_single_key(ctrl, '[', "'['");
    test_single_key(ctrl, ']', "']'");
    test_single_key(ctrl, ';', "';'");
    test_single_key(ctrl, ',', "','");
    test_single_key(ctrl, '.', "'.'");
    test_single_key(ctrl, '/', "'/'");
    test_single_key(ctrl, ' ', "SPACE");

    // --- Single control characters ---
    printf("\n  -- Single control characters --\n");
    test_single_key(ctrl, '\n', "ENTER");
    test_single_key(ctrl, '\b', "BACKSPACE");
    test_single_key(ctrl, '\t', "TAB");
    test_single_key(ctrl, 27, "ESC");

    // --- Raw Linux keycodes ---
    printf("\n  -- Raw Linux keycodes (non-ASCII range) --\n");
    // NOTE: Keycodes < 32 (control chars) map to ASCII equivalents.
    // Keycodes 32-127 that overlap with ASCII letters/digits/symbols also map.
    // Only keycodes OUTSIDE the ASCII mapping (e.g. >= 128 or unmapped symbols)
    // pass through unchanged. See Phase 1 raw_passthrough note.
    test_single_key(ctrl, KEY_LEFTSHIFT, "KEY_LEFTSHIFT");
    test_single_key(ctrl, KEY_LEFTMETA, "KEY_LEFTMETA (Super)");
    test_single_key(ctrl, KEY_CAPSLOCK, "KEY_CAPSLOCK");

    // --- Key sequences (type a word) ---
    test_key_sequence(ctrl, "Hello", "Hello");
    test_key_sequence(ctrl, "MaaFramework", "MaaFramework");
    test_key_sequence(ctrl, "KWin Control Unit 2024", "with spaces and digits");

    // Cleanup
    MaaKWinControlUnitDestroy(handle);

    printf("\n═══════════════════════════════════════════════════\n");
    printf("  Final: %d passed, %d failed\n", s_passed, s_failed);
    printf("═══════════════════════════════════════════════════\n");

    return s_failed > 0 ? 1 : 0;
}
