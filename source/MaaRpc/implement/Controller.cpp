#include "Controller.h"
#include "macro.h"

using namespace ::grpc;

Status ControllerImpl::create_adb(::grpc::ServerContext* context, const ::maarpc::AdbControllerRequest* request,
                                  ::maarpc::HandleResponse* response)
{
    std::ignore = context;

    MAA_GRPC_REQUIRED(callback)
    MAA_GRPC_REQUIRED(adb_path)
    MAA_GRPC_REQUIRED(adb_serial)
    MAA_GRPC_REQUIRED(adb_type)
    MAA_GRPC_REQUIRED(adb_config)

    auto cbId = request->callback().id();
    auto cbState = uImpl->get(cbId);
    if (!cbState) {
        return Status(NOT_FOUND, "id not exists");
    }

    std::unique_lock<std::mutex> lock(handles_mtx);
    auto id = ctrl_id_counter++;
    handles[id] = MaaAdbControllerCreate(request->adb_path().c_str(), request->adb_serial().c_str(),
                                         request->adb_type(), request->adb_config().c_str(), CallbackImpl, cbState);
    lock.unlock();

    response->mutable_value()->set_handle(id);

    return Status::OK;
}

Status ControllerImpl::destroy(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                               ::maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)

    std::unique_lock<std::mutex> lock(handles_mtx);
    auto id = request->handle().handle();
    if (!handles.contains(id)) {
        return Status(NOT_FOUND, "handle not exists");
    }
    auto handle = handles[id];
    handles.erase(id);
    lock.unlock();

    MaaControllerDestroy(handle);

    return Status::OK;
}

Status ControllerImpl::create_custom(::grpc::ServerContext* context, const ::maarpc::CustomControllerRequest* request,
                                     ::grpc::ServerWriter<::maarpc::CustomControllerResponse>* writer)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = writer;

    return Status(UNIMPLEMENTED, "");
}

Status ControllerImpl::submit_custom_controller(::grpc::ServerContext* context,
                                                const ::maarpc::SubmitCustomControllerRequest* request,
                                                ::maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return Status(UNIMPLEMENTED, "");
}

Status ControllerImpl::set_option(::grpc::ServerContext* context, const ::maarpc::ControllerSetOptionRequest* request,
                                  ::maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return Status(UNIMPLEMENTED, "");
}

Status ControllerImpl::post_connection(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                                       ::maarpc::IdResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return Status(UNIMPLEMENTED, "");
}

Status ControllerImpl::post_click(::grpc::ServerContext* context, const ::maarpc::ControllerPostClickRequest* request,
                                  ::maarpc::IdResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return Status(UNIMPLEMENTED, "");
}

Status ControllerImpl::post_swipe(::grpc::ServerContext* context, const ::maarpc::ControllerPostSwipeRequest* request,
                                  ::maarpc::IdResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return Status(UNIMPLEMENTED, "");
}

Status ControllerImpl::post_press_key(::grpc::ServerContext* context, const ::maarpc::ControllerPostKeyRequest* request,
                                      ::maarpc::IdResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return Status(UNIMPLEMENTED, "");
}

Status ControllerImpl::post_touch_down(::grpc::ServerContext* context,
                                       const ::maarpc::ControllerPostTouchRequest* request,
                                       ::maarpc::IdResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return Status(UNIMPLEMENTED, "");
}

Status ControllerImpl::post_touch_move(::grpc::ServerContext* context,
                                       const ::maarpc::ControllerPostTouchRequest* request,
                                       ::maarpc::IdResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return Status(UNIMPLEMENTED, "");
}

Status ControllerImpl::post_touch_up(::grpc::ServerContext* context,
                                     const ::maarpc::ControllerPostTouchRequest* request,
                                     ::maarpc::IdResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return Status(UNIMPLEMENTED, "");
}

Status ControllerImpl::post_screencap(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                                      ::maarpc::IdResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return Status(UNIMPLEMENTED, "");
}

Status ControllerImpl::status(::grpc::ServerContext* context, const ::maarpc::HandleIdRequest* request,
                              ::maarpc::StatusResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return Status(UNIMPLEMENTED, "");
}

Status ControllerImpl::wait(::grpc::ServerContext* context, const ::maarpc::HandleIdRequest* request,
                            ::maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return Status(UNIMPLEMENTED, "");
}

Status ControllerImpl::connected(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                                 ::maarpc::BoolResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return Status(UNIMPLEMENTED, "");
}

Status ControllerImpl::image(::grpc::ServerContext* context, const ::maarpc::ControllerGetImageRequest* request,
                             ::maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return Status(UNIMPLEMENTED, "");
}

Status ControllerImpl::uuid(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                            ::maarpc::StringResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return Status(UNIMPLEMENTED, "");
}
