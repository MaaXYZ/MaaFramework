// Standalone: c++ -std=c++17 -Wall -Wextra -Werror interception_keyboard_selector_test.cpp -o selector_test
#include "../../source/MaaWin32ControlUnit/Input/InterceptionKeyboardSelector.h"

#include <cstdlib>
#include <iostream>

using MaaNS::CtrlUnitNs::InterceptionDetail::KeyboardHardwareIds;
using MaaNS::CtrlUnitNs::InterceptionDetail::select_keyboard;

void check(bool condition, const char* scenario)
{
    if (!condition) {
        std::cerr << scenario << '\n';
        std::exit(1);
    }
}

int main()
{
    KeyboardHardwareIds ids;
    check(!select_keyboard(ids, L"", -1, L""), "Empty driver slots must not be selected");
    ids[2] = L"HID\\KeyboardA";
    check(select_keyboard(ids, L"", -1, L"") == 2, "Skip open but empty preferred slot 1");
    check(!select_keyboard(ids, L"1", -1, L""), "Explicit empty slot must fail without fallback");
    check(select_keyboard(ids, L"2", -1, L"") == 2, "Explicit attached slot");
    check(select_keyboard(ids, L"HID\\KeyboardA", -1, L"") == 2, "Explicit hardware identity");
    check(!select_keyboard(ids, L"10", -1, L""), "Reject out-of-range selector");
    check(!select_keyboard(ids, L"-1", -1, L""), "Reject negative selector");
    check(!select_keyboard(ids, L"missing", -1, L""), "Unknown hardware identity must fail");

    ids[0] = L"HID\\KeyboardB";
    check(select_keyboard(ids, L"", 2, L"HID\\KeyboardA") == 2, "Retain valid binding when another keyboard appears");
    check(select_keyboard(ids, L"0", 2, L"HID\\KeyboardA") == 0, "Explicit selection overrides cached binding");
    ids[2].clear();
    check(select_keyboard(ids, L"", 2, L"HID\\KeyboardA") == 0, "Re-enumerate after disconnection");
    check(!select_keyboard(ids, L"HID\\KeyboardA", 2, L"HID\\KeyboardA"), "Do not substitute an explicitly selected disconnected keyboard");
    ids[4] = L"HID\\KeyboardA";
    check(select_keyboard(ids, L"HID\\KeyboardA", 2, L"HID\\KeyboardA") == 4, "Follow hardware identity to its new slot");
    ids[2] = L"HID\\KeyboardC";
    check(select_keyboard(ids, L"", 2, L"HID\\KeyboardA") == 0, "Do not retain a slot occupied by different hardware");

    for (int slot = 0; slot < 10; ++slot) {
        KeyboardHardwareIds single;
        single[slot] = L"HID\\OnlyKeyboard";
        check(select_keyboard(single, L"", -1, L"") == slot, "Every valid keyboard slot is discoverable");
    }
    std::cout << "Interception keyboard selection tests passed\n";
}
