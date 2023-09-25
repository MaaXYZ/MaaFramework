#include "MaaFrameworkResourceImpl.h"
#include "MaaFramework/MaaAPI.h"

grpc::Status MaaFrameworkResourceImpl::create(grpc::ServerContext* context, const maarpc::IdRequest* request,
                                              maarpc::HandleResponse* response)
{
    std::ignore = context;

    auto cbId = request->id().id();
    auto cbState = fwImpl->get(cbId);
    if (!cbState) {
        return grpc::Status::CANCELLED;
    }

    std::unique_lock<std::mutex> lock(handles_mtx);

    auto id = res_id_counter++;
    handles[id] = MaaResourceCreate(CallbackImpl, cbState);

    lock.unlock();

    response->mutable_value()->set_handle(id);

    return grpc::Status::OK;
}

grpc::Status MaaFrameworkResourceImpl::destroy(grpc::ServerContext* context, const maarpc::HandleRequest* request,
                                               maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = response;

    std::unique_lock<std::mutex> lock(handles_mtx);

    auto id = request->handle().handle();
    if (!handles.contains(id)) {
        return grpc::Status::CANCELLED;
    }

    auto handle = handles[id];
    handles.erase(id);

    lock.unlock();

    MaaResourceDestroy(handle);

    return grpc::Status::OK;
}

grpc::Status MaaFrameworkResourceImpl::post_path(grpc::ServerContext* context,
                                                 const maarpc::HandleStringRequest* request,
                                                 maarpc::IdResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return grpc::Status::CANCELLED;
}

grpc::Status MaaFrameworkResourceImpl::status(grpc::ServerContext* context, const maarpc::HandleIdRequest* request,
                                              maarpc::StatusResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return grpc::Status::CANCELLED;
}

grpc::Status MaaFrameworkResourceImpl::wait(grpc::ServerContext* context, const maarpc::HandleIdRequest* request,
                                            maarpc::StatusResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return grpc::Status::CANCELLED;
}

grpc::Status MaaFrameworkResourceImpl::loaded(grpc::ServerContext* context, const maarpc::HandleRequest* request,
                                              maarpc::BoolResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return grpc::Status::CANCELLED;
}

grpc::Status MaaFrameworkResourceImpl::hash(grpc::ServerContext* context, const maarpc::HandleRequest* request,
                                            maarpc::StringResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return grpc::Status::CANCELLED;
}
