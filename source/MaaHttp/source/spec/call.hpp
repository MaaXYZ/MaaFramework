#pragma once

#include "./type.hpp"
#include "./utils.hpp"

#include "utils/base64.hpp"
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
    reinterpret_cast<void*>(std::stoull(value.as_string()));
return true;
__CALL_DECLARE_JSON_TO_ARG_END()

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
value = lhg::to_base64(data);
res[name] = value;
__CALL_DECLARE_ARG_TO_JSON_END()

#pragma endregion MaaGetImageEncoded

#pragma region MaaSetImageEncoded

__CALL_DECLARE_INPUT(maa::func_type_MaaSetImageEncoded::_2_size, true)

__CALL_DECLARE_JSON_TO_ARG_BEGIN(maa::func_type_MaaSetImageEncoded, _1_data)
std::string& data = std::get<maa::func_type_MaaSetImageEncoded::_1_data::index>(state);
auto opt = lhg::from_base64(value.as_string());
if (!opt.has_value()) {
    return false;
}
data = opt.value();
std::get<maa::func_type_MaaSetImageEncoded::_1_data::index>(arg) =
    reinterpret_cast<unsigned char*>(const_cast<char*>(data.c_str()));
std::get<maa::func_type_MaaSetImageEncoded::_2_size::index>(arg) = data.size();
return true;
__CALL_DECLARE_JSON_TO_ARG_END()

#pragma endregion MaaSetImageEncoded

#define __DECLARE_STRING_BUFFER_OUTPUT(func_tag, arg_tag)                                \
    __CALL_DECLARE_INPUT(func_tag::arg_tag, false)                                       \
    __CALL_DECLARE_OUTPUT(func_tag::arg_tag, true)                                       \
                                                                                         \
    __CALL_DECLARE_PREPARE_STATE_BEGIN(func_tag, arg_tag)                                \
    std::ignore = provider;                                                              \
    std::ignore = req;                                                                   \
    std::ignore = arg;                                                                   \
    std::get<func_tag::arg_tag::index>(state) = MaaCreateStringBuffer();                 \
    std::get<func_tag::arg_tag::index>(arg) = std::get<func_tag::arg_tag::index>(state); \
    return true;                                                                         \
    __CALL_DECLARE_PREPARE_STATE_END()                                                   \
                                                                                         \
    __CALL_DECLARE_ARG_TO_JSON_BEGIN(func_tag, arg_tag)                                  \
    std::ignore = state;                                                                 \
    auto handle = std::get<func_tag::arg_tag::index>(state);                             \
    auto size = MaaGetStringSize(handle);                                                \
    std::string data(MaaGetString(handle), size);                                        \
    MaaDestroyStringBuffer(handle);                                                      \
    res[name] = data;                                                                    \
    __CALL_DECLARE_ARG_TO_JSON_END()

__DECLARE_STRING_BUFFER_OUTPUT(maa::func_type_MaaResourceGetTaskList, _1_buffer)
__DECLARE_STRING_BUFFER_OUTPUT(maa::func_type_MaaResourceGetHash, _1_buffer)
__DECLARE_STRING_BUFFER_OUTPUT(maa::func_type_MaaControllerGetUUID, _1_buffer)
__DECLARE_STRING_BUFFER_OUTPUT(maa::func_type_MaaSyncContextGetTaskResult, _2_out_task_result)
__DECLARE_STRING_BUFFER_OUTPUT(maa::func_type_MaaSyncContextRunRecognizer, _5_out_detail)

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
    __CALL_DECLARE_JSON_TO_ARG_END()

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
static MaaCustomRecognizerAPI api = {
    callback::create_callback<maa::callback_CustomRecognizerAnalyze>()
};
std::get<maa::func_type_MaaRegisterCustomRecognizer::_2_recognizer::index>(arg) = &api;
std::get<maa::func_type_MaaRegisterCustomRecognizer::_3_recognizer_arg::index>(arg) = ctx.get();
return true;
__CALL_DECLARE_JSON_TO_ARG_END()

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
ctx->run = run_ctx;
ctx->stop = stop_ctx;
static MaaCustomActionAPI api = { callback::create_callback<maa::callback_CustomActionRun>(),
                                  callback::create_callback<maa::callback_CustomActionStop>() };
std::get<maa::func_type_MaaRegisterCustomAction::_2_action::index>(arg) = &api;
std::get<maa::func_type_MaaRegisterCustomAction::_3_action_arg::index>(arg) = ctx;
return true;
__CALL_DECLARE_JSON_TO_ARG_END()

__CALL_DECLARE_JSON_TO_ARG_BEGIN(maa::func_type_MaaSyncContextRunAction, _3_cur_box)
auto rec = &std::get<maa::func_type_MaaSyncContextRunAction::_3_cur_box::index>(state);
rec->x = value.at("x").as_integer();
rec->y = value.at("y").as_integer();
rec->width = value.at("width").as_integer();
rec->height = value.at("height").as_integer();
return true;
__CALL_DECLARE_JSON_TO_ARG_END()

__CALL_DECLARE_INPUT(maa::func_type_MaaSyncContextRunRecognizer::_4_out_box, false)
__CALL_DECLARE_OUTPUT(maa::func_type_MaaSyncContextRunRecognizer::_4_out_box, true)

__CALL_DECLARE_PREPARE_STATE_BEGIN(maa::func_type_MaaSyncContextRunRecognizer, _4_out_box)
std::ignore = provider;
std::ignore = req;
std::ignore = arg;
std::ignore = state;
return true;
__CALL_DECLARE_PREPARE_STATE_END()

__CALL_DECLARE_ARG_TO_JSON_BEGIN(maa::func_type_MaaSyncContextRunRecognizer, _4_out_box)
auto rec = std::get<maa::func_type_MaaSyncContextRunRecognizer::_4_out_box::index>(state);
res[name] = {
    { "x", rec.x },
    { "y", rec.y },
    { "width", rec.width },
    { "height", rec.height },
};
__CALL_DECLARE_ARG_TO_JSON_END()

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

__CALL_DECLARE_ARG_TO_JSON_SCHEMA_TYPE(maa::func_type_MaaGetImageEncoded, ret, "string")
__CALL_DECLARE_JSON_TO_ARG_SCHEMA_TYPE(maa::func_type_MaaSetImageEncoded, _1_data, "string")

__CALL_DECLARE_ARG_TO_JSON_SCHEMA_TYPE(maa::func_type_MaaToolkitGetWindow, ret, "string")
__CALL_DECLARE_ARG_TO_JSON_SCHEMA_TYPE(maa::func_type_MaaToolkitGetCursorWindow, ret, "string")
__CALL_DECLARE_ARG_TO_JSON_SCHEMA_TYPE(maa::func_type_MaaToolkitGetDesktopWindow, ret, "string")
__CALL_DECLARE_ARG_TO_JSON_SCHEMA_TYPE(maa::func_type_MaaToolkitGetForegroundWindow, ret, "string")

__CALL_DECLARE_ARG_TO_JSON_SCHEMA_TYPE(maa::func_type_MaaResourceGetTaskList, _1_buffer, "string")
__CALL_DECLARE_ARG_TO_JSON_SCHEMA_TYPE(maa::func_type_MaaResourceGetHash, _1_buffer, "string")
__CALL_DECLARE_ARG_TO_JSON_SCHEMA_TYPE(maa::func_type_MaaControllerGetUUID, _1_buffer, "string")
__CALL_DECLARE_ARG_TO_JSON_SCHEMA_TYPE(
    maa::func_type_MaaSyncContextGetTaskResult,
    _2_out_task_result,
    "string")
__CALL_DECLARE_ARG_TO_JSON_SCHEMA_TYPE(
    maa::func_type_MaaSyncContextRunRecognizer,
    _5_out_detail,
    "string")

__CALL_DECLARE_JSON_TO_ARG_SCHEMA_TYPE(maa::func_type_MaaWin32ControllerCreate, _0_hWnd, "string")

__CALL_DECLARE_JSON_TO_ARG_SCHEMA_TYPE(
    maa::func_type_MaaWin32ControllerCreate,
    _2_callback,
    "string")
__CALL_DECLARE_JSON_TO_ARG_SCHEMA_TYPE(
    maa::func_type_MaaAdbControllerCreateV2,
    _5_callback,
    "string")
__CALL_DECLARE_JSON_TO_ARG_SCHEMA_TYPE(
    maa::func_type_MaaThriftControllerCreate,
    _4_callback,
    "string")
__CALL_DECLARE_JSON_TO_ARG_SCHEMA_TYPE(maa::func_type_MaaDbgControllerCreate, _4_callback, "string")
__CALL_DECLARE_JSON_TO_ARG_SCHEMA_TYPE(maa::func_type_MaaResourceCreate, _0_callback, "string")
__CALL_DECLARE_JSON_TO_ARG_SCHEMA_TYPE(maa::func_type_MaaCreate, _0_callback, "string")

__CALL_DECLARE_JSON_TO_ARG_SCHEMA_TYPE(
    maa::func_type_MaaRegisterCustomRecognizer,
    _2_recognizer,
    "string")

__CALL_DECLARE_JSON_TO_ARG_SCHEMA_BEGIN(maa::func_type_MaaRegisterCustomAction, _2_action)
b.type("object").prop({
    { "run", schema::Builder().type("string").obj },
    { "stop", schema::Builder().type("string").obj },
});
__CALL_DECLARE_JSON_TO_ARG_SCHEMA_END()

__CALL_DECLARE_JSON_TO_ARG_SCHEMA_BEGIN(maa::func_type_MaaSyncContextRunAction, _3_cur_box)
b.type("object").prop({ { "x", schema::Builder().type("number").obj },
                        { "y", schema::Builder().type("number").obj },
                        { "width", schema::Builder().type("number").obj },
                        { "height", schema::Builder().type("number").obj } });
__CALL_DECLARE_JSON_TO_ARG_SCHEMA_END()

__CALL_DECLARE_ARG_TO_JSON_SCHEMA_BEGIN(maa::func_type_MaaSyncContextRunRecognizer, _4_out_box)
b.type("object").prop({ { "x", schema::Builder().type("number").obj },
                        { "y", schema::Builder().type("number").obj },
                        { "width", schema::Builder().type("number").obj },
                        { "height", schema::Builder().type("number").obj } });
__CALL_DECLARE_ARG_TO_JSON_SCHEMA_END()

}; // namespace lhg::call
