#include "Instance.h"
#include "MaaFramework/Task/MaaCustomAction.h"
#include "MaaFramework/Task/MaaCustomRecognizer.h"
#include "macro.h"

using namespace ::grpc;

Status InstanceImpl::create(ServerContext* context, const ::maarpc::IdRequest* request,
                            ::maarpc::HandleResponse* response)
{
    std::ignore = context;

    MAA_GRPC_REQUIRED(id)

    auto cbId = request->id();
    UtilityImpl::CallbackState* cbState;

    if (!uImpl->states.get(cbId, cbState)) {
        return Status(NOT_FOUND, "id not exists");
    }

    auto id = make_uuid();
    handles.add(id, MaaCreate(CallbackImpl, cbState));

    response->set_handle(id);

    return Status::OK;
}

Status InstanceImpl::destroy(ServerContext* context, const ::maarpc::HandleRequest* request,
                             ::maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_DEL_HANDLE

    MaaDestroy(handle);

    return Status::OK;
}

static MaaBool _analyze(MaaSyncContextHandle sync_context, const MaaImageBufferHandle image, MaaStringView task_name,
                        MaaStringView custom_recognition_param, MaaRectHandle out_box,
                        MaaStringBufferHandle detail_buff, MaaTransparentArg arg)
{
    auto info = reinterpret_cast<InstanceImpl::CustomRecognizerInfo*>(arg);
    auto stream = info->stream;

    ::maarpc::CustomRecognizerResponse response;

    auto sid = make_uuid();
    info->sImpl->handles.add(sid, sync_context);
    response.mutable_analyze()->set_context(sid);

    auto iid = make_uuid();
    info->iImpl->handles.add(iid, image);
    response.mutable_analyze()->set_image_handle(iid);

    response.mutable_analyze()->set_task(task_name);
    response.mutable_analyze()->set_param(custom_recognition_param);

    stream->Write(response);

    ::maarpc::CustomRecognizerRequest request;
    stream->Read(&request);

    info->sImpl->handles.del(sid);
    info->iImpl->handles.del(iid);

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

static MaaCustomRecognizerAPI customRecognizerApi = { _analyze };

Status InstanceImpl::register_custom_recognizer(
    ServerContext* context,
    ServerReaderWriter<::maarpc::CustomRecognizerResponse, ::maarpc::CustomRecognizerRequest>* stream)
{
    std::ignore = context;

    ::maarpc::CustomRecognizerRequest requestData;
    auto request = &requestData;

    if (!stream->Read(request)) {
        return Status(FAILED_PRECONDITION, "custom recognizer cannot read init");
    }

    MAA_GRPC_REQUIRED_CASE_AS(result, Init)

    MAA_GRPC_GET_HANDLE_FROM(this, handle, init().handle)

    CustomRecognizerInfo info { request->init().name(), stream, iImpl, sImpl };

    ::maarpc::CustomRecognizerResponse response;
    stream->Write(response);

    MaaRegisterCustomRecognizer(handle, request->init().name().c_str(), &customRecognizerApi, &info);

    recos.add(request->init().name(), &info);
    recoIdx.add(&info, handle);

    info.finish.acquire();

    return Status::OK;
}

Status InstanceImpl::unregister_custom_recognizer(ServerContext* context, const ::maarpc::HandleStringRequest* request,
                                                  ::maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(str)

    MAA_GRPC_GET_HANDLE

    MaaUnregisterCustomRecognizer(handle, request->str().c_str());

    CustomRecognizerInfo* info = nullptr;
    if (recos.get(request->str(), info)) {
        recoIdx.del(info);
        info->finish.release();
    }

    return Status::OK;
}

Status InstanceImpl::clear_custom_recognizer(ServerContext* context, const ::maarpc::HandleRequest* request,
                                             ::maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    MaaClearCustomRecognizer(handle);

    std::vector<CustomRecognizerInfo*> infos;
    if (recoIdx.find_all(handle, infos)) {
        for (auto info : infos) {
            recoIdx.del(info);
            recos.del(info->name);
            info->finish.release();
        }
    }

    return Status::OK;
}

static MaaBool _run(MaaSyncContextHandle sync_context, MaaStringView task_name, MaaStringView custom_action_param,
                    MaaRectHandle cur_box, MaaStringView cur_rec_detail, MaaTransparentArg arg)
{
    auto info = reinterpret_cast<InstanceImpl::CustomActionInfo*>(arg);
    auto stream = info->stream;

    ::maarpc::CustomActionResponse response;

    auto sid = make_uuid();
    info->sImpl->handles.add(sid, sync_context);
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

    info->sImpl->handles.del(sid);

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

static MaaCustomActionAPI customActionApi = { _run, _stop };

Status InstanceImpl::register_custom_action(
    ServerContext* context, ServerReaderWriter<::maarpc::CustomActionResponse, ::maarpc::CustomActionRequest>* stream)
{
    std::ignore = context;

    ::maarpc::CustomActionRequest requestData;
    auto request = &requestData;

    if (!stream->Read(request)) {
        return Status(FAILED_PRECONDITION, "custom action cannot read init");
    }

    MAA_GRPC_REQUIRED_CASE_AS(result, Init)

    MAA_GRPC_GET_HANDLE_FROM(this, handle, init().handle)

    CustomActionInfo info { request->init().name(), stream, sImpl };

    ::maarpc::CustomActionResponse response;
    stream->Write(response);

    MaaRegisterCustomAction(handle, request->init().name().c_str(), &customActionApi, &info);

    actions.add(request->init().name(), &info);
    actionIdx.add(&info, handle);

    info.finish.acquire();

    return Status::OK;
}

Status InstanceImpl::unregister_custom_action(ServerContext* context, const ::maarpc::HandleStringRequest* request,
                                              ::maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(str)

    MAA_GRPC_GET_HANDLE

    MaaUnregisterCustomAction(handle, request->str().c_str());

    CustomActionInfo* info = nullptr;
    if (actions.get(request->str(), info)) {
        actionIdx.del(info);
        info->finish.release();
    }

    return Status::OK;
}

Status InstanceImpl::clear_custom_action(ServerContext* context, const ::maarpc::HandleRequest* request,
                                         ::maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    MaaClearCustomAction(handle);

    std::vector<CustomActionInfo*> infos;
    if (actionIdx.find_all(handle, infos)) {
        for (auto info : infos) {
            actionIdx.del(info);
            actions.del(info->name);
            info->finish.release();
        }
    }

    return Status::OK;
}

Status InstanceImpl::bind_resource(ServerContext* context, const ::maarpc::HandleHandleRequest* request,
                                   ::maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(another_handle)

    MAA_GRPC_GET_HANDLE
    MAA_GRPC_GET_HANDLE_FROM(rImpl, res_handle, another_handle)

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
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(another_handle)

    MAA_GRPC_GET_HANDLE
    MAA_GRPC_GET_HANDLE_FROM(cImpl, ctrl_handle, another_handle)

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
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    response->set_bool_(MaaInited(handle));

    return Status::OK;
}

Status InstanceImpl::post_task(ServerContext* context, const ::maarpc::InstancePostTaskRequest* request,
                               ::maarpc::IIdResponse* response)
{
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
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    response->set_bool_(MaaTaskAllFinished(handle));

    return Status::OK;
}

Status InstanceImpl::stop(ServerContext* context, const ::maarpc::HandleRequest* request,
                          ::maarpc::EmptyResponse* response)
{
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
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    std::string id;
    if (rImpl->handles.find(MaaGetResource(handle), id)) {
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
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    std::string id;
    if (cImpl->handles.find(MaaGetController(handle), id)) {
        response->set_handle(id);
        return Status::OK;
    }
    else {
        return Status(NOT_FOUND, "cannot locate controller handle");
    }
}
