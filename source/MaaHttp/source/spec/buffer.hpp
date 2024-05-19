#pragma once

#include "type.hpp"

#include "utils/phony.hpp"

#include "../info.hpp"

namespace lhg::lifecycle
{

// MaaCreateImageBuffer

#define CURF maa::func_type_MaaCreateImageBuffer

LHGHandle(CURF::ret, alloc);

#undef CURF

// DestroyImageBuffer

#define CURF maa::func_type_MaaDestroyImageBuffer

LHGHandle(CURF::_0_handle, free);

#undef CURF

// MaaGetImageEncoded

#define CURF maa::func_type_MaaGetImageEncoded

LHGInherit (arg_to_json, CURF::ret, true) {
    void convert()
    {
        auto size = MaaGetImageEncodedSize(std::get<_0_handle::index>(arg));
        std::string_view data(reinterpret_cast<char*>(std::get<index>(arg)), size);
        res[name] = to_base64(data);
    }
};

LHGSchema(arg_to_json_schema, CURF::ret, true, "string");

#undef CURF

// MaaSetImageEncoded

#define CURF maa::func_type_MaaSetImageEncoded

LHGHide(CURF::_2_size);
LHGOuterState(CURF::_1_data, std::string);

LHGInherit (json_to_arg, CURF::_1_data, true) {
    bool convert()
    {
        if (!req.contains(name)) {
            return false;
        }
        auto buffer = from_base64(req.at(name).as_string());
        if (!buffer.has_value()) {
            return false;
        }
        auto& v = std::get<index>(state);
        v = std::move(buffer.value());
        std::get<index>(arg) = reinterpret_cast<unsigned char*>(v.data());
        std::get<_2_size::index>(arg) = v.size();
        return true;
    }
};

LHGSchema(json_to_arg_schema, CURF::_1_data, true, "string");

#undef CURF

// MaaCreateImageListBuffer

#define CURF maa::func_type_MaaCreateImageListBuffer

LHGHandle(CURF::ret, alloc);

#undef CURF

// MaaDestroyImageListBuffer

#define CURF maa::func_type_MaaDestroyImageListBuffer

LHGHandle(CURF::_0_handle, free);

#undef CURF

// MaaGetImageListAt

#define CURF maa::func_type_MaaGetImageListAt

LHGHandle(CURF::ret, temp);

#undef CURF

// MaaResourceGetTaskList

#define CURF maa::func_type_MaaResourceGetTaskList

LHGOutput(CURF::_1_buffer);
LHGOuterState(CURF::_1_buffer, MaaStringBufferHandle);

LHGInherit (pre_process, CURF::_1_buffer) {
    void process()
    {
        std::get<index>(state) = MaaCreateStringBuffer();
    }
};

LHGInherit (post_process, CURF::_1_buffer) {
    void process()
    {
        MaaDestroyStringBuffer(std::get<index>(state));
    }
};

LHGInherit (arg_to_json, CURF::_1_buffer, true) {
    void convert()
    {
        auto handle = std::get<index>(state);
        auto size = MaaGetStringSize(handle);
        std::string data(MaaGetString(handle), size);
        res[name] = data;
    }
};

LHGSchema(arg_to_json_schema, CURF::_1_buffer, true, "string");

#undef CURF

}
