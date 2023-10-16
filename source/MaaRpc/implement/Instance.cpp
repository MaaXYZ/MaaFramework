#include "Instance.h"
#include "MaaFramework/MaaAPI.h"
#include "Macro.h"
#include "Utils/Logger.h"

using namespace ::grpc;

Status InstanceImpl::create(ServerContext* context, const ::maarpc::IdRequest* request,
                            ::maarpc::HandleResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(id)

    auto cb_id = request->id();
    std::shared_ptr<UtilityImpl::CallbackState> cb_state = nullptr;

    if (!utility_impl_->states().get(cb_id, cb_state)) {
        return Status(NOT_FOUND, "id not exists");
    }

    auto id = make_uuid();
    handles_.add(id, MaaCreate(callback_impl, cb_state.get()));

    response->set_handle(id);

    return Status::OK;
}

Status InstanceImpl::destroy(ServerContext* context, const ::maarpc::HandleRequest* request,
                             ::maarpc::EmptyResponse* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_DEL_HANDLE

    MaaDestroy(handle);

    return Status::OK;
}

static MaaBool _analyze(MaaSyncContextHandle sync_context, const MaaImageBufferHandle image, MaaStringView task_name,
                        MaaStringView custom_recognition_param, MaaTransparentArg recognizer_arg, MaaRectHandle out_box,
                        MaaStringBufferHandle detail_buff)
{
    auto info = reinterpret_cast<InstanceImpl::CustomRecognizerInfo*>(recognizer_arg);
    auto stream = info->stream;

    ::maarpc::CustomRecognizerResponse response;

    auto sid = make_uuid();
    info->syncctx_impl->handles().add(sid, sync_context);
    response.mutable_analyze()->set_context(sid);

    auto iid = make_uuid();
    info->image_impl->handles().add(iid, image);
    response.mutable_analyze()->set_image_handle(iid);

    response.mutable_analyze()->set_task(task_name);
    response.mutable_analyze()->set_param(custom_recognition_param);

    stream->Write(response);

    ::maarpc::CustomRecognizerRequest request;
    stream->Read(&request);

    info->syncctx_impl->handles().del(sid);
    info->image_impl->handles().del(iid);

    if (request.result_case() != ::maarpc::CustomRecognizerRequest::kAnalyze) {
        return false;
    }

    const auto& box = request.analyze().box();
    out_box->x = box.xy().x();
    out_box->y = box.xy().y();
    out_box->width = box.wh().width();
    out_box->height = box.wh().height();

    const auto& dt = request.analyze().detail();
    MaaSetStringEx(detail_buff, dt.c_str(), dt.size());

    return request.ok() && request.analyze().match();
}

static MaaCustomRecognizerAPI custom_recognizer_api = { _analyze };

Status InstanceImpl::register_custom_recognizer(
    ServerContext* context,
    ServerReaderWriter<::maarpc::CustomRecognizerResponse, ::maarpc::CustomRecognizerRequest>* stream)
{
    LogFunc;
    std::ignore = context;

    ::maarpc::CustomRecognizerRequest request_data;
    auto request = &request_data;

    if (!stream->Read(request)) {
        return Status(FAILED_PRECONDITION, "custom recognizer cannot read init");
    }

    MAA_GRPC_REQUIRED_CASE_AS(result, Init)

    MAA_GRPC_GET_HANDLE_FROM(this, handle, init().handle)

#if defined(__APPLE__) && defined(__clang__) && __clang_major__ < 16
    std::shared_ptr<CustomRecognizerInfo> info(
        new CustomRecognizerInfo { request->init().name(), stream, image_impl_, syncctx_impl_ });
#else
    // build error of Apple clang version 15.0.0 (clang-1500.0.40.1) on macOS
    auto info = std::make_shared<CustomRecognizerInfo>(request->init().name(), stream, image_impl_, syncctx_impl_);
#endif

    ::maarpc::CustomRecognizerResponse response;
    stream->Write(response);

    MaaRegisterCustomRecognizer(handle, request->init().name().c_str(), &custom_recognizer_api, info.get());

    recos_.add(request->init().name(), info);
    reco_idx_.add(info, handle);

    info->finish.acquire();

    return Status::OK;
}

Status InstanceImpl::unregister_custom_recognizer(ServerContext* context, const ::maarpc::HandleStringRequest* request,
                                                  ::maarpc::EmptyResponse* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(str)

    MAA_GRPC_GET_HANDLE

    MaaUnregisterCustomRecognizer(handle, request->str().c_str());

    std::shared_ptr<CustomRecognizerInfo> info = nullptr;
    if (recos_.get(request->str(), info)) {
        reco_idx_.del(info);
        info->finish.release();
    }

    return Status::OK;
}

Status InstanceImpl::clear_custom_recognizer(ServerContext* context, const ::maarpc::HandleRequest* request,
                                             ::maarpc::EmptyResponse* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    MaaClearCustomRecognizer(handle);

    std::vector<std::shared_ptr<CustomRecognizerInfo>> infos;
    if (reco_idx_.find_all(handle, infos)) {
        for (auto& info : infos) {
            reco_idx_.del(info);
            recos_.del(info->name);
            info->finish.release();
        }
    }

    return Status::OK;
}

static MaaBool _run(MaaSyncContextHandle sync_context, MaaStringView task_name, MaaStringView custom_action_param,
                    MaaRectHandle cur_box, MaaStringView cur_rec_detail, MaaTransparentArg action_arg)
{
    auto info = reinterpret_cast<InstanceImpl::CustomActionInfo*>(action_arg);
    auto stream = info->stream;

    ::maarpc::CustomActionResponse response;

    auto sid = make_uuid();
    info->syncctx_impl->handles().add(sid, sync_context);
    response.mutable_run()->set_context(sid);

    response.mutable_run()->set_task(task_name);
    response.mutable_run()->set_param(custom_action_param);

    response.mutable_run()->mutable_box()->mutable_xy()->set_x(cur_box->x);
    response.mutable_run()->mutable_box()->mutable_xy()->set_y(cur_box->y);
    response.mutable_run()->mutable_box()->mutable_wh()->set_width(cur_box->width);
    response.mutable_run()->mutable_box()->mutable_wh()->set_height(cur_box->height);

    response.mutable_run()->set_detail(cur_rec_detail);

    stream->Write(response);

    ::maarpc::CustomActionRequest request;
    stream->Read(&request);

    info->syncctx_impl->handles().del(sid);

    return request.ok();
}

static void _stop(MaaTransparentArg arg)
{
    auto info = reinterpret_cast<InstanceImpl::CustomActionInfo*>(arg);
    auto stream = info->stream;

    ::maarpc::CustomActionResponse response;

    response.set_stop(true);

    stream->Write(response);

    ::maarpc::CustomActionRequest request;
    stream->Read(&request);
}

static MaaCustomActionAPI custom_action_api = { _run, _stop };

Status InstanceImpl::register_custom_action(
    ServerContext* context, ServerReaderWriter<::maarpc::CustomActionResponse, ::maarpc::CustomActionRequest>* stream)
{
    LogFunc;
    std::ignore = context;

    ::maarpc::CustomActionRequest requestData;
    auto request = &requestData;

    if (!stream->Read(request)) {
        return Status(FAILED_PRECONDITION, "custom action cannot read init");
    }

    MAA_GRPC_REQUIRED_CASE_AS(result, Init)

    MAA_GRPC_GET_HANDLE_FROM(this, handle, init().handle)

#if defined(__APPLE__) && defined(__clang__) && __clang_major__ < 16
    std::shared_ptr<CustomActionInfo> info(new CustomActionInfo { request->init().name(), stream, syncctx_impl_ });
#else
    // build error of Apple clang version 15.0.0 (clang-1500.0.40.1) on macOS
    auto info = std::make_shared<CustomActionInfo>(request->init().name(), stream, syncctx_impl_);
#endif

    ::maarpc::CustomActionResponse response;
    stream->Write(response);

    MaaRegisterCustomAction(handle, request->init().name().c_str(), &custom_action_api, info.get());

    actions_.add(request->init().name(), info);
    action_idx_.add(info, handle);

    info->finish.acquire();

    return Status::OK;
}

Status InstanceImpl::unregister_custom_action(ServerContext* context, const ::maarpc::HandleStringRequest* request,
                                              ::maarpc::EmptyResponse* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(str)

    MAA_GRPC_GET_HANDLE

    MaaUnregisterCustomAction(handle, request->str().c_str());

    std::shared_ptr<CustomActionInfo> info = nullptr;
    if (actions_.get(request->str(), info)) {
        action_idx_.del(info);
        info->finish.release();
    }

    return Status::OK;
}

Status InstanceImpl::clear_custom_action(ServerContext* context, const ::maarpc::HandleRequest* request,
                                         ::maarpc::EmptyResponse* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    MaaClearCustomAction(handle);

    std::vector<std::shared_ptr<CustomActionInfo>> infos;
    if (action_idx_.find_all(handle, infos)) {
        for (auto& info : infos) {
            action_idx_.del(info);
            actions_.del(info->name);
            info->finish.release();
        }
    }

    return Status::OK;
}

Status InstanceImpl::bind_resource(ServerContext* context, const ::maarpc::HandleHandleRequest* request,
                                   ::maarpc::EmptyResponse* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(another_handle)

    MAA_GRPC_GET_HANDLE
    MAA_GRPC_GET_HANDLE_FROM(resource_impl_, res_handle, another_handle)

    if (MaaBindResource(handle, res_handle)) {
        return Status::OK;
    }
    else {
        return Status(UNKNOWN, "MaaBindResource failed");
    }
}

Status InstanceImpl::bind_controller(ServerContext* context, const ::maarpc::HandleHandleRequest* request,
                                     ::maarpc::EmptyResponse* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(another_handle)

    MAA_GRPC_GET_HANDLE
    MAA_GRPC_GET_HANDLE_FROM(controller_impl_, ctrl_handle, another_handle)

    if (MaaBindController(handle, ctrl_handle)) {
        return Status::OK;
    }
    else {
        return Status(UNKNOWN, "MaaBindController failed");
    }
}

Status InstanceImpl::inited(ServerContext* context, const ::maarpc::HandleRequest* request,
                            ::maarpc::BoolResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    response->set_bool_(MaaInited(handle));

    return Status::OK;
}

Status InstanceImpl::post_task(ServerContext* context, const ::maarpc::InstancePostTaskRequest* request,
                               ::maarpc::IIdResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(task)
    MAA_GRPC_REQUIRED(param)

    MAA_GRPC_GET_HANDLE

    response->set_id(MaaPostTask(handle, request->task().c_str(), request->param().c_str()));

    return Status::OK;
}

Status InstanceImpl::set_task_param(ServerContext* context, const ::maarpc::InstanceSetTaskParamRequest* request,
                                    ::maarpc::EmptyResponse* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(id)
    MAA_GRPC_REQUIRED(param)

    MAA_GRPC_GET_HANDLE

    if (MaaSetTaskParam(handle, request->id(), request->param().c_str())) {
        return Status::OK;
    }
    else {
        return Status(UNKNOWN, "MaaSetTaskParam failed");
    }
}

Status InstanceImpl::status(ServerContext* context, const ::maarpc::HandleIIdRequest* request,
                            ::maarpc::StatusResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(id)

    MAA_GRPC_GET_HANDLE

    response->set_status(MaaTaskStatus(handle, request->id()));

    return Status::OK;
}

Status InstanceImpl::wait(ServerContext* context, const ::maarpc::HandleIIdRequest* request,
                          ::maarpc::StatusResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(id)

    MAA_GRPC_GET_HANDLE

    response->set_status(MaaWaitTask(handle, request->id()));

    return Status::OK;
}

Status InstanceImpl::all_finished(ServerContext* context, const ::maarpc::HandleRequest* request,
                                  ::maarpc::BoolResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    response->set_bool_(MaaTaskAllFinished(handle));

    return Status::OK;
}

Status InstanceImpl::stop(ServerContext* context, const ::maarpc::HandleRequest* request,
                          ::maarpc::EmptyResponse* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    if (MaaStop(handle)) {
        return Status::OK;
    }
    else {
        return Status(UNKNOWN, "MaaStop failed");
    }
}

Status InstanceImpl::resource(ServerContext* context, const ::maarpc::HandleRequest* request,
                              ::maarpc::HandleRequest* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    std::string id;
    if (resource_impl_->handles().find(MaaGetResource(handle), id)) {
        response->set_handle(id);
        return Status::OK;
    }
    else {
        return Status(NOT_FOUND, "cannot locate resource handle");
    }
}

Status InstanceImpl::controller(ServerContext* context, const ::maarpc::HandleRequest* request,
                                ::maarpc::HandleRequest* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    std::string id;
    if (controller_impl_->handles().find(MaaGetController(handle), id)) {
        response->set_handle(id);
        return Status::OK;
    }
    else {
        return Status(NOT_FOUND, "cannot locate controller handle");
    }
}
