#include "Instance.h"
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

Status InstanceImpl::register_custom_recognizer(
    ServerContext* context,
    ServerReaderWriter<::maarpc::CustomRecognizerResponse, ::maarpc::CustomRecognizerRequest>* stream)
{
    std::ignore = context;
    std::ignore = stream;

    return Status(UNIMPLEMENTED, "");
}

Status InstanceImpl::unregister_custom_recognizer(ServerContext* context, const ::maarpc::HandleStringRequest* request,
                                                  ::maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return Status(UNIMPLEMENTED, "");
}

Status InstanceImpl::clear_custom_recognizer(ServerContext* context, const ::maarpc::HandleRequest* request,
                                             ::maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return Status(UNIMPLEMENTED, "");
}

Status InstanceImpl::register_custom_action(
    ServerContext* context, ServerReaderWriter<::maarpc::CustomActionResponse, ::maarpc::CustomActionRequest>* stream)
{
    std::ignore = context;
    std::ignore = stream;

    return Status(UNIMPLEMENTED, "");
}

Status InstanceImpl::unregister_custom_action(ServerContext* context, const ::maarpc::HandleStringRequest* request,
                                              ::maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return Status(UNIMPLEMENTED, "");
}

Status InstanceImpl::clear_custom_action(ServerContext* context, const ::maarpc::HandleRequest* request,
                                         ::maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return Status(UNIMPLEMENTED, "");
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
                               ::maarpc::IdResponse* response)
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
