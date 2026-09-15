#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>

namespace MaaNS::CtrlUnitNs::InterceptionDetail
{

using KeyboardHardwareIds = std::array<std::wstring, 10>;

// A driver endpoint can be opened even when no keyboard occupies its slot.
inline std::optional<int> select_keyboard(
    const KeyboardHardwareIds& hardware_ids,
    std::wstring_view selector,
    int previous_index,
    std::wstring_view previous_hardware_id)
{
    if (!selector.empty()) {
        if (selector.size() == 1 && selector.front() >= L'0' && selector.front() <= L'9') {
            const int index = selector.front() - L'0';
            return hardware_ids[index].empty() ? std::nullopt : std::optional<int>(index);
        }
        for (int index = 0; index < static_cast<int>(hardware_ids.size()); ++index) {
            if (!hardware_ids[index].empty() && hardware_ids[index] == selector) {
                return index;
            }
        }
        return std::nullopt;
    }
    if (previous_index >= 0 && previous_index < static_cast<int>(hardware_ids.size())
        && !previous_hardware_id.empty() && hardware_ids[previous_index] == previous_hardware_id) {
        return previous_index;
    }
    for (int index = 0; index < static_cast<int>(hardware_ids.size()); ++index) {
        if (!hardware_ids[index].empty()) {
            return index;
        }
    }
    return std::nullopt;
}

} // namespace MaaNS::CtrlUnitNs::InterceptionDetail
