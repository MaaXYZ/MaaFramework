#include "Win32HeldInput.h"

#include <iostream>
#include <vector>

#include "../../../source/MaaWin32ControlUnit/Base/HeldInput.h"

bool test_win32_held_input()
{
    MAA_CTRL_UNIT_NS::HeldInput keys;
    constexpr int first = 0x86;
    constexpr int second = 0x87;
    constexpr int unowned = 0x85;
    keys.down(first, true);
    keys.down(second, true);
    keys.down(unowned, false);

    std::vector<int> releases;
    const auto release = [&](int code) {
        releases.push_back(code);
        return true;
    };
    if (keys.up(unowned, false, release) || !releases.empty()) {
        std::cerr << "A lost window released an input whose down failed\n";
        return false;
    }
    if (!keys.up(first, false, release) || releases != std::vector { first }) {
        std::cerr << "Stopping after window loss did not release the owned input\n";
        return false;
    }
    if (keys.up(first, false, release) || releases.size() != 1) {
        std::cerr << "The same owned input was released twice after window loss\n";
        return false;
    }

    if (keys.release_all([](int) { return false; }) || !keys.contains(second)) {
        std::cerr << "A failed release forgot the outstanding input\n";
        return false;
    }
    if (!keys.release_all(release) || !keys.release_all(release) || releases != std::vector { first, second }) {
        std::cerr << "Rebinding should drain each outstanding input exactly once\n";
        return false;
    }
    return true;
}
