#pragma once

#include "./type.hpp"

#include "utils/macro.hpp"
#include "utils/phony.hpp"

#include "../info.hpp"

LHGArgHandle(maa::func_type_MaaWin32ControllerCreate::ret, alloc);
LHGArgHandle(maa::func_type_MaaAdbControllerCreateV2::ret, alloc);
LHGArgHandle(maa::func_type_MaaThriftControllerCreate::ret, alloc);
LHGArgHandle(maa::func_type_MaaDbgControllerCreate::ret, alloc);
LHGArgHandle(maa::func_type_MaaControllerDestroy::_0_ctrl, free);

LHGArgHandle(maa::func_type_MaaResourceCreate::ret, alloc);
LHGArgHandle(maa::func_type_MaaResourceDestroy::_0_res, free);

LHGArgHandle(maa::func_type_MaaCreate::ret, alloc);
LHGArgHandle(maa::func_type_MaaDestroy::_0_inst, free);

LHGArgHandle(maa::func_type_MaaCreateImageBuffer::ret, alloc);
LHGArgHandle(maa::func_type_MaaDestroyImageBuffer::_0_handle, free);

LHGArgHandle(maa::func_type_MaaCreateImageListBuffer::ret, alloc);
LHGArgHandle(maa::func_type_MaaDestroyImageListBuffer::_0_handle, free);
LHGArgHandle(maa::func_type_MaaGetImageListAt::ret, temp);

#define ReturnHwnd(func_tag)                                                             \
    LHGArgToJsonBegin(func_tag, ret, true)                                               \
        value = std::format("{:#018x}", reinterpret_cast<size_t>(std::get<index>(arg))); \
        LHGArgToJsonMiddle()                                                             \
        b.type("string");                                                                \
    LHGArgToJsonEnd()

ReturnHwnd(maa::func_type_MaaToolkitGetWindow);
ReturnHwnd(maa::func_type_MaaToolkitGetCursorWindow);
ReturnHwnd(maa::func_type_MaaToolkitGetDesktopWindow);
ReturnHwnd(maa::func_type_MaaToolkitGetForegroundWindow);

#undef ReturnHwnd

LHGArgFromJsonBegin(maa::func_type_MaaWin32ControllerCreate, _0_hWnd, true)
    std::get<index>(arg) = reinterpret_cast<void*>(std::stoull(value.as_string(), nullptr, 0));
    LHGArgFromJsonMiddle()
    b.type("string");
LHGArgFromJsonEnd()

LHGArgFromJsonBegin(maa::func_type_MaaToolkitGetWindowClassName, _0_hwnd, true)
    std::get<index>(arg) = reinterpret_cast<void*>(std::stoull(value.as_string(), nullptr, 0));
    LHGArgFromJsonMiddle()
    b.type("string");
LHGArgFromJsonEnd()

LHGArgFromJsonBegin(maa::func_type_MaaToolkitGetWindowWindowName, _0_hwnd, true)
    std::get<index>(arg) = reinterpret_cast<void*>(std::stoull(value.as_string(), nullptr, 0));
    LHGArgFromJsonMiddle()
    b.type("string");
LHGArgFromJsonEnd()

LHGArgToJsonBegin(maa::func_type_MaaGetImageEncoded, ret, true)
    auto size =
        MaaGetImageEncodedSize(std::get<maa::func_type_MaaGetImageEncoded::_0_handle::index>(arg));
    std::string_view data(reinterpret_cast<char*>(std::get<index>(arg)), size);
    value = to_base64(data);
    LHGArgToJsonMiddle()
    b.type("string");
LHGArgToJsonEnd()

LHGArgHide(maa::func_type_MaaSetImageEncoded::_2_size);
LHGArgFromJsonBegin(maa::func_type_MaaSetImageEncoded, _1_data, true)
    std::string& data = std::get<index>(state);
    auto opt = from_base64(value.as_string());
    if (!opt.has_value()) {
        return false;
    }
    data = opt.value();
    std::get<index>(arg) = reinterpret_cast<unsigned char*>(const_cast<char*>(data.c_str()));
    std::get<maa::func_type_MaaSetImageEncoded::_2_size::index>(arg) = data.size();
    LHGArgFromJsonMiddle()
    b.type("string");
LHGArgFromJsonEnd()

#define OutputString(func_tag, atag)                                                               \
    LHGArgOutput(func_tag::atag);                                                                  \
    LHGArgPrepareStateBegin(func_tag, atag)                                                        \
        std::get<arg_tag::index>(arg) = std::get<arg_tag::index>(state) = MaaCreateStringBuffer(); \
    LHGArgPrepareStateEnd()                                                                        \
    LHGArgToJsonBegin(func_tag, atag, true)                                                        \
        auto handle = std::get<func_tag::atag::index>(state);                                      \
        auto size = MaaGetStringSize(handle);                                                      \
        std::string data(MaaGetString(handle), size);                                              \
        MaaDestroyStringBuffer(handle);                                                            \
        value = data;                                                                              \
        LHGArgToJsonMiddle()                                                                       \
        b.type("string");                                                                          \
    LHGArgToJsonEnd()

OutputString(maa::func_type_MaaResourceGetTaskList, _1_buffer);
OutputString(maa::func_type_MaaResourceGetHash, _1_buffer);
OutputString(maa::func_type_MaaControllerGetUUID, _1_buffer);
OutputString(maa::func_type_MaaQueryRecognitionDetail, _3_detail_json);
OutputString(maa::func_type_MaaSyncContextRunRecognition, _5_out_detail);
OutputString(maa::func_type_MaaToolkitGetWindowClassName, _1_buffer);
OutputString(maa::func_type_MaaToolkitGetWindowWindowName, _1_buffer);

#undef OutputString

#define ApiCallback(func_tag, cb_tag, ctx_tag)                                              \
    LHGArgHide(func_tag::ctx_tag);                                                          \
    LHGArgFromJsonBegin(func_tag, cb_tag, true)                                             \
        std::string id = value.as_string();                                                 \
        auto manager = provider.get<CallbackManager<maa::callback_MaaAPICallback>, void>(); \
        auto ctx = manager->query(id);                                                      \
        if (!ctx.get()) {                                                                   \
            return false;                                                                   \
        }                                                                                   \
        std::get<index>(arg) = callback::create_callback<maa::callback_MaaAPICallback>();   \
        std::get<func_tag::ctx_tag::index>(arg) = ctx.get();                                \
        LHGArgFromJsonMiddle()                                                              \
        b.type("string");                                                                   \
    LHGArgFromJsonEnd()

ApiCallback(maa::func_type_MaaWin32ControllerCreate, _2_callback, _3_callback_arg);
ApiCallback(maa::func_type_MaaAdbControllerCreateV2, _5_callback, _6_callback_arg);
ApiCallback(maa::func_type_MaaThriftControllerCreate, _4_callback, _5_callback_arg);
ApiCallback(maa::func_type_MaaDbgControllerCreate, _4_callback, _5_callback_arg);
ApiCallback(maa::func_type_MaaResourceCreate, _0_callback, _1_callback_arg);
ApiCallback(maa::func_type_MaaCreate, _0_callback, _1_callback_arg);

#undef ApiCallback

LHGArgHide(maa::func_type_MaaRegisterCustomRecognizer::_3_recognizer_arg);
LHGArgFromJsonBegin(maa::func_type_MaaRegisterCustomRecognizer, _2_recognizer, true)
    std::string id = value.as_string();
    auto manager = provider.get<CallbackManager<maa::callback_CustomRecognizerAnalyze>, void>();
    auto ctx = manager->query(id);
    if (!ctx.get()) {
        return false;
    }
    static MaaCustomRecognizerAPI api = {
        callback::create_callback<maa::callback_CustomRecognizerAnalyze>()
    };
    std::get<index>(arg) = &api;
    std::get<maa::func_type_MaaRegisterCustomRecognizer::_3_recognizer_arg::index>(arg) = ctx.get();
    LHGArgFromJsonMiddle()
    b.type("string");
LHGArgFromJsonEnd()

LHGArgHide(maa::func_type_MaaRegisterCustomAction::_3_action_arg);
LHGArgFromJsonBegin(maa::func_type_MaaRegisterCustomAction, _2_action, true)
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
    std::get<index>(arg) = &api;
    std::get<maa::func_type_MaaRegisterCustomAction::_3_action_arg::index>(arg) = ctx;
    LHGArgFromJsonMiddle()
    b.type("object").prop({ { "run", schema::Builder().type("string").obj },
                            { "stop", schema::Builder().type("string").obj } });
LHGArgFromJsonEnd()

LHGArgOutput(maa::func_type_MaaQueryRecognitionDetail::_1_hit);
LHGArgPrepareStateBegin(maa::func_type_MaaQueryRecognitionDetail, _1_hit)
    std::get<arg_tag::index>(arg) = &std::get<arg_tag::index>(state);
LHGArgPrepareStateEnd()
LHGArgToJsonBegin(maa::func_type_MaaQueryRecognitionDetail, _1_hit, true)
    value = std::get<index>(state);
    LHGArgToJsonMiddle()
    b.type("boolean");
LHGArgToJsonEnd()

LHGArgOutput(maa::func_type_MaaQueryRecognitionDetail::_2_hit_box);
LHGArgPrepareStateBegin(maa::func_type_MaaQueryRecognitionDetail, _2_hit_box)
    std::get<arg_tag::index>(arg) = &std::get<arg_tag::index>(state);
LHGArgPrepareStateEnd()
LHGArgToJsonBegin(maa::func_type_MaaQueryRecognitionDetail, _2_hit_box, true)
    auto rec = &std::get<index>(state);
    value = pri_maa::from_rect(rec);
    LHGArgToJsonMiddle()
    pri_maa::schema_rect(b);
LHGArgToJsonEnd()

LHGArgFromJsonBegin(maa::func_type_MaaSyncContextRunAction, _3_cur_box, true)
    auto rec = &std::get<index>(state);
    pri_maa::to_rect(value, rec);
    LHGArgFromJsonMiddle()
    pri_maa::schema_rect(b);
LHGArgFromJsonEnd()

LHGArgOutput(maa::func_type_MaaQueryNodeDetail::_1_reco_id);
LHGArgPrepareStateBegin(maa::func_type_MaaQueryNodeDetail, _1_reco_id)
    std::get<arg_tag::index>(arg) = &std::get<arg_tag::index>(state);
LHGArgPrepareStateEnd()
LHGArgToJsonBegin(maa::func_type_MaaQueryNodeDetail, _1_reco_id, true)
    value = std::get<index>(state);
    LHGArgToJsonMiddle()
    b.type("number");
LHGArgToJsonEnd()

LHGArgOutput(maa::func_type_MaaQueryNodeDetail::_2_run_completed);
LHGArgPrepareStateBegin(maa::func_type_MaaQueryNodeDetail, _2_run_completed)
    std::get<arg_tag::index>(arg) = &std::get<arg_tag::index>(state);
LHGArgPrepareStateEnd()
LHGArgToJsonBegin(maa::func_type_MaaQueryNodeDetail, _2_run_completed, true)
    value = std::get<index>(state);
    LHGArgToJsonMiddle()
    b.type("boolean");
LHGArgToJsonEnd()

LHGArgOutput(maa::func_type_MaaSyncContextRunRecognition::_4_out_box);
LHGArgPrepareStateBegin(maa::func_type_MaaSyncContextRunRecognition, _4_out_box)
    std::get<arg_tag::index>(arg) = &std::get<arg_tag::index>(state);
LHGArgPrepareStateEnd()
LHGArgToJsonBegin(maa::func_type_MaaSyncContextRunRecognition, _4_out_box, true)
    auto rec = &std::get<index>(state);
    value = pri_maa::from_rect(rec);
    LHGArgToJsonMiddle()
    pri_maa::schema_rect(b);
LHGArgToJsonEnd()

LHGArgHide(maa::func_type_MaaToolkitRegisterCustomRecognizerExecutor::_4_exec_param_size);
LHGArgFromJsonBegin(maa::func_type_MaaToolkitRegisterCustomRecognizerExecutor, _3_exec_params, true)
    auto& [vecstr, vecptr] = std::get<index>(state);
    for (const auto& str : value.as_array()) {
        vecstr.push_back(str.as_string());
        vecptr.push_back(const_cast<MaaStringView>(vecstr.back().c_str()));
    }
    std::get<index>(arg) = vecptr.data();
    std::get<maa::func_type_MaaToolkitRegisterCustomRecognizerExecutor::_4_exec_param_size::index>(
        arg) = vecptr.size();
    LHGArgFromJsonMiddle()
    b.type("array").items(schema::Builder().type("string").obj);
LHGArgFromJsonEnd()

LHGArgHide(maa::func_type_MaaToolkitRegisterCustomActionExecutor::_4_exec_param_size);
LHGArgFromJsonBegin(maa::func_type_MaaToolkitRegisterCustomActionExecutor, _3_exec_params, true)
    auto& [vecstr, vecptr] = std::get<index>(state);
    for (const auto& str : value.as_array()) {
        vecstr.push_back(str.as_string());
        vecptr.push_back(const_cast<MaaStringView>(vecstr.back().c_str()));
    }
    std::get<index>(arg) = vecptr.data();
    std::get<maa::func_type_MaaToolkitRegisterCustomActionExecutor::_4_exec_param_size::index>(
        arg) = vecptr.size();
    LHGArgFromJsonMiddle()
    b.type("array").items(schema::Builder().type("string").obj);
LHGArgFromJsonEnd()

LHGArgOutput(maa::func_type_MaaQueryTaskDetail::_1_node_id_list);
LHGArgHide(maa::func_type_MaaQueryTaskDetail::_2_node_id_list_size);
LHGArgPrepareStateBegin(maa::func_type_MaaQueryTaskDetail, _1_node_id_list)
    MaaSize sz;
    MaaQueryTaskDetail(
        std::get<maa::func_type_MaaQueryTaskDetail::_0_task_id::index>(arg),
        nullptr,
        &sz);
    std::get<index>(state).resize(sz);
    std::get<index>(arg) = std::get<index>(state).data();
    std::get<maa::func_type_MaaQueryTaskDetail::_2_node_id_list_size::index>(state) = sz;
    std::get<maa::func_type_MaaQueryTaskDetail::_2_node_id_list_size::index>(arg) =
        &std::get<maa::func_type_MaaQueryTaskDetail::_2_node_id_list_size::index>(state);
LHGArgPrepareStateEnd()
LHGArgToJsonBegin(maa::func_type_MaaQueryTaskDetail, _1_node_id_list, true)
    value = std::get<index>(state);
    LHGArgToJsonMiddle()
    b.type("array").items(schema::Builder().type("number").obj);
LHGArgToJsonEnd()
