#pragma once

#include "./type.hpp"
#include "./utils.hpp"

#include "../base64.hpp"
#include "utils/phony.hpp"

#include "../info.hpp"

namespace lhg::call
{

#pragma region hwnd

#define __DECLARE_RETURN_HWND(func_tag)                                                         \
    __CALL_DECLARE_ARG_TO_JSON_BEGIN(func_tag, ret)                                             \
    std::ignore = state;                                                                        \
    json::value value;                                                                          \
    value =                                                                                     \
        std::format("{:#018x}", reinterpret_cast<size_t>(std::get<func_tag::ret::index>(arg))); \
    res[name] = value;                                                                          \
    __CALL_DECLARE_ARG_TO_JSON_END()

__DECLARE_RETURN_HWND(maa::func_type_MaaToolkitGetWindow)
__DECLARE_RETURN_HWND(maa::func_type_MaaToolkitGetCursorWindow)
__DECLARE_RETURN_HWND(maa::func_type_MaaToolkitGetDesktopWindow)
__DECLARE_RETURN_HWND(maa::func_type_MaaToolkitGetForegroundWindow)

#undef __DECLARE_RETURN_HWND

__CALL_DECLARE_JSON_TO_ARG_BEGIN(maa::func_type_MaaWin32ControllerCreate, _0_hWnd)
std::get<maa::func_type_MaaWin32ControllerCreate::_0_hWnd::index>(arg) =
    reinterpret_cast<void*>(value.as_unsigned_long_long());
return true;
__DECLARE_JSON_TO_ARG_END()

#pragma endregion hwnd

#pragma region MaaGetImageEncoded

__CALL_DECLARE_ARG_TO_JSON_BEGIN(maa::func_type_MaaGetImageEncoded, ret)
std::ignore = state;
json::value value;
auto size =
    MaaGetImageEncodedSize(std::get<maa::func_type_MaaGetImageEncoded::_0_handle::index>(arg));
std::string_view data(
    reinterpret_cast<char*>(std::get<maa::func_type_MaaGetImageEncoded::ret::index>(arg)),
    size);
value = base64::to_base64(data);
res[name] = value;
__CALL_DECLARE_ARG_TO_JSON_END()

#pragma endregion MaaGetImageEncoded

#pragma region MaaSetImageEncoded

__CALL_DECLARE_INPUT(maa::func_type_MaaSetImageEncoded::_2_size, true)

__CALL_DECLARE_JSON_TO_ARG_BEGIN(maa::func_type_MaaSetImageEncoded, _1_data)
std::string& data = std::get<maa::func_type_MaaSetImageEncoded::_1_data::index>(state);
data = base64::from_base64(value.as_string());
std::get<maa::func_type_MaaSetImageEncoded::_1_data::index>(arg) =
    reinterpret_cast<unsigned char*>(const_cast<char*>(data.c_str()));
std::get<maa::func_type_MaaSetImageEncoded::_2_size::index>(arg) = data.size();
return true;
__DECLARE_JSON_TO_ARG_END()

#pragma endregion MaaSetImageEncoded

#define __DECLARE_STRING_BUFFER_OUTPUT(func_tag, arg_tag)                \
    __CALL_DECLARE_INPUT(func_tag::arg_tag, false)                       \
    __CALL_DECLARE_OUTPUT(func_tag::arg_tag, true)                       \
                                                                         \
    __CALL_DECLARE_PREPARE_STATE_BEGIN(func_tag, arg_tag)                \
    std::ignore = provider;                                              \
    std::ignore = req;                                                   \
    std::ignore = arg;                                                   \
    std::get<func_tag::arg_tag::index>(state) = MaaCreateStringBuffer(); \
    return true;                                                         \
    __CALL_DECLARE_PREPARE_STATE_END()                                   \
                                                                         \
    __CALL_DECLARE_ARG_TO_JSON_BEGIN(func_tag, arg_tag)                  \
    std::ignore = state;                                                 \
    json::value value;                                                   \
    auto handle = std::get<func_tag::arg_tag::index>(state);             \
    auto size = MaaGetStringSize(handle);                                \
    std::string data(MaaGetString(handle), size);                        \
    res[name] = value;                                                   \
    __CALL_DECLARE_ARG_TO_JSON_END()

__DECLARE_STRING_BUFFER_OUTPUT(maa::func_type_MaaResourceGetTaskList, _1_buffer)
__DECLARE_STRING_BUFFER_OUTPUT(maa::func_type_MaaResourceGetHash, _1_buffer)
__DECLARE_STRING_BUFFER_OUTPUT(maa::func_type_MaaControllerGetUUID, _1_buffer)

#undef __DECLARE_STRING_BUFFER_OUTPUT

#define __DECLARE_APICALLBACK(func_tag, cb_tag, ctx_tag)                                \
    __CALL_DECLARE_INPUT(func_tag::ctx_tag, false)                                      \
                                                                                        \
    __CALL_DECLARE_JSON_TO_ARG_BEGIN(func_tag, cb_tag)                                  \
    std::string id = value.as_string();                                                 \
    auto manager = provider.get<CallbackManager<maa::callback_MaaAPICallback>, void>(); \
    auto ctx = manager->query(id);                                                      \
    if (!ctx.get()) {                                                                   \
        return false;                                                                   \
    }                                                                                   \
    std::get<func_tag::cb_tag::index>(arg) =                                            \
        callback::create_callback<maa::callback_MaaAPICallback>();                      \
    std::get<func_tag::ctx_tag::index>(arg) = ctx.get();                                \
    return true;                                                                        \
    __DECLARE_JSON_TO_ARG_END()

__DECLARE_APICALLBACK(maa::func_type_MaaAdbControllerCreate, _4_callback, _5_callback_arg)
__DECLARE_APICALLBACK(maa::func_type_MaaWin32ControllerCreate, _2_callback, _3_callback_arg)
__DECLARE_APICALLBACK(maa::func_type_MaaAdbControllerCreateV2, _5_callback, _6_callback_arg)
__DECLARE_APICALLBACK(maa::func_type_MaaThriftControllerCreate, _4_callback, _5_callback_arg)
__DECLARE_APICALLBACK(maa::func_type_MaaDbgControllerCreate, _4_callback, _5_callback_arg)
__DECLARE_APICALLBACK(maa::func_type_MaaResourceCreate, _0_callback, _1_callback_arg)
__DECLARE_APICALLBACK(maa::func_type_MaaCreate, _0_callback, _1_callback_arg)

#undef __DECLARE_APICALLBACK

__CALL_DECLARE_INPUT(maa::func_type_MaaRegisterCustomRecognizer::_3_recognizer_arg, false)

__CALL_DECLARE_JSON_TO_ARG_BEGIN(maa::func_type_MaaRegisterCustomRecognizer, _2_recognizer)
std::string id = value.as_string();
auto manager = provider.get<CallbackManager<maa::callback_CustomRecognizerAnalyze>, void>();
auto ctx = manager->query(id);
if (!ctx.get()) {
    return false;
}
MaaCustomRecognizerAPI api = { callback::create_callback<maa::callback_CustomRecognizerAnalyze>() };
std::get<maa::func_type_MaaRegisterCustomRecognizer::_2_recognizer::index>(arg) = &api;
std::get<maa::func_type_MaaRegisterCustomRecognizer::_3_recognizer_arg::index>(arg) = ctx.get();
return true;
__DECLARE_JSON_TO_ARG_END()

__CALL_DECLARE_INPUT(maa::func_type_MaaRegisterCustomAction::_3_action_arg, false)

__CALL_DECLARE_JSON_TO_ARG_BEGIN(maa::func_type_MaaRegisterCustomAction, _2_action)
json::object ids = value.as_object();
std::string run_id = ids["run"].as_string();
std::string stop_id = ids["stop"].as_string();
auto run_manager = provider.get<CallbackManager<maa::callback_CustomActionRun>, void>();
auto run_ctx = run_manager->query(run_id);
if (!run_ctx.get()) {
    return false;
}
auto stop_manager = provider.get<CallbackManager<maa::callback_CustomActionStop>, void>();
auto stop_ctx = stop_manager->query(stop_id);
if (!stop_ctx.get()) {
    return false;
}
auto ctx = provider.get<DataManager, void>()->alloc<pri_maa::custom_action_context>(
    std::vector<std::string> { run_id, stop_id });
MaaCustomActionAPI api = { callback::create_callback<maa::callback_CustomActionRun>(),
                           callback::create_callback<maa::callback_CustomActionStop>() };
std::get<maa::func_type_MaaRegisterCustomAction::_2_action::index>(arg) = &api;
std::get<maa::func_type_MaaRegisterCustomAction::_3_action_arg::index>(arg) = ctx;
return true;
__DECLARE_JSON_TO_ARG_END()

__CALL_DECLARE_HANDLE_OPER(maa::func_type_MaaAdbControllerCreate::ret, alloc)
__CALL_DECLARE_HANDLE_OPER(maa::func_type_MaaWin32ControllerCreate::ret, alloc)
__CALL_DECLARE_HANDLE_OPER(maa::func_type_MaaAdbControllerCreateV2::ret, alloc)
__CALL_DECLARE_HANDLE_OPER(maa::func_type_MaaThriftControllerCreate::ret, alloc)
__CALL_DECLARE_HANDLE_OPER(maa::func_type_MaaDbgControllerCreate::ret, alloc)
__CALL_DECLARE_HANDLE_OPER(maa::func_type_MaaControllerDestroy::_0_ctrl, free)

__CALL_DECLARE_HANDLE_OPER(maa::func_type_MaaResourceCreate::ret, alloc)
__CALL_DECLARE_HANDLE_OPER(maa::func_type_MaaResourceDestroy::_0_res, free)

__CALL_DECLARE_HANDLE_OPER(maa::func_type_MaaCreate::ret, alloc)
__CALL_DECLARE_HANDLE_OPER(maa::func_type_MaaDestroy::_0_inst, free)

__CALL_DECLARE_HANDLE_OPER(maa::func_type_MaaCreateImageBuffer::ret, alloc)
__CALL_DECLARE_HANDLE_OPER(maa::func_type_MaaDestroyImageBuffer::_0_handle, free)

}; // namespace lhg::call
