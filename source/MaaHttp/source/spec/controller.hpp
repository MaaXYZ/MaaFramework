#pragma once

#include "helper.hpp"
#include "type.hpp"

#include "utils/phony.hpp"

#include "../info.hpp"

namespace lhg::lifecycle
{

// MaaAdbControllerCreateV2

#define CURF maa::func_type_MaaAdbControllerCreateV2

LHGHandle(CURF::ret, alloc);

LHGCallback(maa::callback_MaaAPICallback, CURF::_5_callback, CURF::_6_callback_arg, true);

#undef CURF

// MaaWin32ControllerCreate

#define CURF maa::func_type_MaaWin32ControllerCreate

LHGHandle(CURF::ret, alloc);

LHGInherit (json_to_arg, CURF::_0_hWnd, true) {
    bool convert()
    {
        if (!req.contains(name)) {
            return false;
        }
        std::get<index>(arg) =
            reinterpret_cast<void*>(std::stoull(req.at(name).as_string(), nullptr, 0));
        return true;
    }
};

LHGSchema(json_to_arg_schema, CURF::_0_hWnd, true, "string");

LHGCallback(maa::callback_MaaAPICallback, CURF::_2_callback, CURF::_3_callback_arg, true);

#undef CURF

// MaaThriftControllerCreate

#define CURF maa::func_type_MaaThriftControllerCreate

LHGHandle(CURF::ret, alloc);

LHGCallback(maa::callback_MaaAPICallback, CURF::_4_callback, CURF::_5_callback_arg, true);

#undef CURF

// MaaDbgControllerCreate

#define CURF maa::func_type_MaaDbgControllerCreate

LHGHandle(CURF::ret, alloc);

LHGCallback(maa::callback_MaaAPICallback, CURF::_4_callback, CURF::_5_callback_arg, true);

#undef CURF

// MaaControllerDestroy

#define CURF maa::func_type_MaaControllerDestroy

LHGHandle(CURF::_0_ctrl, free);

#undef CURF

// MaaControllerGetUUID

#define CURF maa::func_type_MaaControllerGetUUID

LHGOuterState(CURF::_1_buffer, MaaStringBufferHandle);
MaaDeclStringBuffer(_1_buffer, true);

#undef CURF

}

