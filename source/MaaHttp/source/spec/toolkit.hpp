#pragma once

#include "helper.hpp"
#include "type.hpp"

#include "utils/phony.hpp"

#include "../info.hpp"

namespace lhg::lifecycle
{

// MaaToolkitGetWindow

#define CURF maa::func_type_MaaToolkitGetWindow

MaaDeclPointerOut(ret, true);

#undef CURF

// MaaToolkitGetCursorWindow

#define CURF maa::func_type_MaaToolkitGetCursorWindow

MaaDeclPointerOut(ret, true);

#undef CURF

// MaaToolkitGetDesktopWindow

#define CURF maa::func_type_MaaToolkitGetDesktopWindow

MaaDeclPointerOut(ret, true);

#undef CURF

// MaaToolkitGetForegroundWindow

#define CURF maa::func_type_MaaToolkitGetForegroundWindow

MaaDeclPointerOut(ret, true);

#undef CURF

// MaaToolkitGetWindowClassName

#define CURF maa::func_type_MaaToolkitGetWindowClassName

LHGOuterState(CURF::_1_buffer, MaaStringBufferHandle);

MaaDeclPointerIn(_0_hwnd, true);
MaaDeclStringBuffer(_1_buffer, true);

#undef CURF

// MaaToolkitGetWindowWindowName

#define CURF maa::func_type_MaaToolkitGetWindowWindowName

LHGOuterState(CURF::_1_buffer, MaaStringBufferHandle);

MaaDeclPointerIn(_0_hwnd, true);
MaaDeclStringBuffer(_1_buffer, true);

#undef CURF

using vec_str_helper_state = std::tuple<std::vector<std::string>, std::vector<MaaStringView>>;

// MaaToolkitRegisterCustomRecognizerExecutor

#define CURF maa::func_type_MaaToolkitRegisterCustomRecognizerExecutor

LHGHide(CURF::_4_exec_param_size);
LHGOuterState(CURF::_3_exec_params, vec_str_helper_state);

LHGInherit (json_to_arg, CURF::_3_exec_params, true) {
    bool convert()
    {
        if (!req.contains(name)) {
            return false;
        }
        auto& [vecstr, vecptr] = std::get<index>(state);
        for (const auto& str : req.at(name).as_array()) {
            vecstr.push_back(str.as_string());
            vecptr.push_back(vecstr.back().c_str());
        }
        std::get<index>(arg) = vecptr.data();
        std::get<_4_exec_param_size::index>(arg) = vecptr.size();
        return true;
    }
};

LHGInherit (json_to_arg_schema, CURF::_3_exec_params, true) {
    void schema()
    {
        b.type("array").items(lhg::schema::Builder().type("string").obj);
        res[name] = b.obj;
    }
};

#undef CURF

// MaaToolkitRegisterCustomActionExecutor

#define CURF maa::func_type_MaaToolkitRegisterCustomActionExecutor

LHGHide(CURF::_4_exec_param_size);
LHGOuterState(CURF::_3_exec_params, vec_str_helper_state);

LHGInherit (json_to_arg, CURF::_3_exec_params, true) {
    bool convert()
    {
        if (!req.contains(name)) {
            return false;
        }
        auto& [vecstr, vecptr] = std::get<index>(state);
        for (const auto& str : req.at(name).as_array()) {
            vecstr.push_back(str.as_string());
            vecptr.push_back(vecstr.back().c_str());
        }
        std::get<index>(arg) = vecptr.data();
        std::get<_4_exec_param_size::index>(arg) = vecptr.size();
        return true;
    }
};

LHGInherit (json_to_arg_schema, CURF::_3_exec_params, true) {
    void schema()
    {
        b.type("array").items(lhg::schema::Builder().type("string").obj);
        res[name] = b.obj;
    }
};

#undef CURF
}
