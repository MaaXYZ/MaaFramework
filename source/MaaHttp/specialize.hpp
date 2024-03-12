#pragma once

#include "MaaFramework/MaaDef.h"
#include "base64.hpp"

#include "MaaFramework/Utility/MaaBuffer.h"
#include "function/interface.hpp"
#include "info.hpp"
#include "utils/phony.hpp"

namespace lhg::call
{

#define __DECLARE_INPUT(arg_tag, v)      \
    template <>                          \
    struct is_input<arg_tag, true>       \
    {                                    \
        constexpr static bool value = v; \
    };

#define __DECLARE_OUTPUT(arg_tag, v)     \
    template <>                          \
    struct is_output<arg_tag, true>      \
    {                                    \
        constexpr static bool value = v; \
    };

#define __DECLARE_OUTER_STATE(arg_tag, t)   \
    template <>                             \
    struct is_outer_state<arg_tag, true>    \
    {                                       \
        constexpr static bool value = true; \
        using type = t;                     \
    };

#define __DECLARE_PREPARE_STATE_BEGIN(func_tag, atag)                                                     \
    template <>                                                                                           \
    struct prepare_state<typename func_tag::args, func_tag::atag::index, true>                            \
    {                                                                                                     \
        using arg_tuple = func_tag::args;                                                                 \
        using arg_tag = func_tag::atag;                                                                   \
        using call_arg_tuple = convert_arg_type<arg_tuple>;                                               \
        using call_arg_state_tuple = convert_outer_state<arg_tuple>;                                      \
        static bool convert(lhg::ManagerProvider& provider, const json::object& req, call_arg_tuple& arg, \
                            call_arg_state_tuple& state)                                                  \
        {

#define __DECLARE_PREPARE_STATE_END() \
    }                                 \
    }                                 \
    ;

#define __DECLARE_JSON_TO_ARG_BEGIN(func_tag, atag)                                                       \
    template <>                                                                                           \
    struct json_to_arg<typename func_tag::args, func_tag::atag::index, true>                              \
    {                                                                                                     \
        using arg_tuple = func_tag::args;                                                                 \
        using arg_tag = func_tag::atag;                                                                   \
        using call_arg_tuple = convert_arg_type<arg_tuple>;                                               \
        using call_arg_state_tuple = convert_outer_state<arg_tuple>;                                      \
        static bool convert(lhg::ManagerProvider& provider, const json::object& req, call_arg_tuple& arg, \
                            call_arg_state_tuple& state)                                                  \
        {                                                                                                 \
            if constexpr (!is_input<arg_tag, true>::value) {                                              \
                return true;                                                                              \
            }                                                                                             \
            else {                                                                                        \
                auto name = arg_tag::name;                                                                \
                if (!req.contains(name)) {                                                                \
                    return false;                                                                         \
                }                                                                                         \
                const json::value& value = req.at(name);
#define __DECLARE_JSON_TO_ARG_END() \
    }                               \
    }                               \
    }                               \
    ;

#define __DECLARE_ARG_TO_JSON_BEGIN(func_tag, atag)                                                 \
    template <>                                                                                     \
    struct arg_to_json<typename func_tag::args, func_tag::atag::index, true>                        \
    {                                                                                               \
        using arg_tuple = func_tag::args;                                                           \
        using arg_tag = func_tag::atag;                                                             \
        using call_arg_tuple = convert_arg_type<arg_tuple>;                                         \
        using call_arg_state_tuple = convert_outer_state<arg_tuple>;                                \
        static void convert(lhg::ManagerProvider& provider, json::object& res, call_arg_tuple& arg, \
                            call_arg_state_tuple& state)                                            \
        {                                                                                           \
            if constexpr (!is_output<arg_tag, true>::value) {                                       \
                return;                                                                             \
            }                                                                                       \
            else {                                                                                  \
                auto name = arg_tag::name;
#define __DECLARE_ARG_TO_JSON_END() \
    }                               \
    }                               \
    }                               \
    ;

#define __DECLARE_RETURN_HWND_ARG_TO_JSON(func_tag)                                                 \
    __DECLARE_ARG_TO_JSON_BEGIN(func_tag, ret)                                                      \
    std::ignore = state;                                                                            \
    json::value value;                                                                              \
    value = std::format("{:#018x}", reinterpret_cast<size_t>(std::get<func_tag::ret::index>(arg))); \
    res[name] = value;                                                                              \
    __DECLARE_ARG_TO_JSON_END()

__DECLARE_RETURN_HWND_ARG_TO_JSON(maa::func_type_MaaToolkitGetWindow)
__DECLARE_RETURN_HWND_ARG_TO_JSON(maa::func_type_MaaToolkitGetCursorWindow)
__DECLARE_RETURN_HWND_ARG_TO_JSON(maa::func_type_MaaToolkitGetDesktopWindow)
__DECLARE_RETURN_HWND_ARG_TO_JSON(maa::func_type_MaaToolkitGetForegroundWindow)

#pragma region MaaGetImageEncoded

__DECLARE_ARG_TO_JSON_BEGIN(maa::func_type_MaaGetImageEncoded, ret)
std::ignore = state;
json::value value;
auto size = MaaGetImageEncodedSize(std::get<maa::func_type_MaaGetImageEncoded::_0_handle::index>(arg));
std::string_view data(reinterpret_cast<char*>(std::get<maa::func_type_MaaGetImageEncoded::ret::index>(arg)), size);
value = base64::to_base64(data);
res[name] = value;
__DECLARE_ARG_TO_JSON_END()

#pragma endregion MaaGetImageEncoded

#pragma region MaaSetImageEncoded

__DECLARE_INPUT(maa::func_type_MaaSetImageEncoded::_2_size, true)
__DECLARE_OUTER_STATE(maa::func_type_MaaSetImageEncoded::_1_data, std::string)

__DECLARE_JSON_TO_ARG_BEGIN(maa::func_type_MaaSetImageEncoded, _1_data)
std::string& data = std::get<maa::func_type_MaaSetImageEncoded::_1_data::index>(state);
data = base64::from_base64(value.as_string());
std::get<maa::func_type_MaaSetImageEncoded::_1_data::index>(arg) =
    reinterpret_cast<unsigned char*>(const_cast<char*>(data.c_str()));
std::get<maa::func_type_MaaSetImageEncoded::_2_size::index>(arg) = data.size();
return true;
__DECLARE_JSON_TO_ARG_END()

#pragma endregion MaaSetImageEncoded

#define __DECLARE_STRING_BUFFER_OUTPUT(func_tag, arg_tag)                \
    __DECLARE_INPUT(func_tag::arg_tag, false)                            \
    __DECLARE_OUTPUT(func_tag::arg_tag, true)                            \
    __DECLARE_OUTER_STATE(func_tag::arg_tag, MaaStringBufferHandle)      \
                                                                         \
    __DECLARE_PREPARE_STATE_BEGIN(func_tag, arg_tag)                     \
    std::ignore = provider;                                              \
    std::ignore = req;                                                   \
    std::ignore = arg;                                                   \
    std::get<func_tag::arg_tag::index>(state) = MaaCreateStringBuffer(); \
    return true;                                                         \
    __DECLARE_PREPARE_STATE_END()                                        \
                                                                         \
    __DECLARE_ARG_TO_JSON_BEGIN(func_tag, arg_tag)                       \
    std::ignore = state;                                                 \
    json::value value;                                                   \
    auto handle = std::get<func_tag::arg_tag::index>(state);             \
    auto size = MaaGetStringSize(handle);                                \
    std::string data(MaaGetString(handle), size);                        \
    res[name] = value;                                                   \
    __DECLARE_ARG_TO_JSON_END()

__DECLARE_STRING_BUFFER_OUTPUT(maa::func_type_MaaResourceGetTaskList, _1_buffer)
__DECLARE_STRING_BUFFER_OUTPUT(maa::func_type_MaaResourceGetHash, _1_buffer)

}; // namespace lhg::call
