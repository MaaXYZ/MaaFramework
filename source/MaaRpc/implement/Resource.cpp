#include "Resource.h"
#include "MaaFramework/MaaAPI.h"

using namespace ::grpc;

Status ResourceImpl::create(ServerContext* context, const ::maarpc::IdRequest* request,
                            ::maarpc::HandleResponse* response)
{
    std::ignore = context;

    if (!request->has_id()) {
        return Status(INVALID_ARGUMENT, "id not provided");
    }

    auto cbId = request->id().id();
    auto cbState = uImpl->get(cbId);
    if (!cbState) {
        return Status(NOT_FOUND, "id not exists");
    }

    std::unique_lock<std::mutex> lock(handles_mtx);
    auto id = res_id_counter++;
    handles[id] = MaaResourceCreate(CallbackImpl, cbState);
    lock.unlock();

    response->mutable_value()->set_handle(id);

    return Status::OK;
}

Status ResourceImpl::destroy(ServerContext* context, const ::maarpc::HandleRequest* request,
                             ::maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = response;

    if (!request->has_handle()) {
        return Status(INVALID_ARGUMENT, "handle not provided");
    }

    std::unique_lock<std::mutex> lock(handles_mtx);
    auto id = request->handle().handle();
    if (!handles.contains(id)) {
        return Status(NOT_FOUND, "handle not exists");
    }
    auto handle = handles[id];
    handles.erase(id);
    lock.unlock();

    MaaResourceDestroy(handle);

    return Status::OK;
}

Status ResourceImpl::post_path(ServerContext* context, const ::maarpc::HandleStringRequest* request,
                               ::maarpc::IdResponse* response)
{
    std::ignore = context;

    if (!request->has_handle()) {
        return Status(INVALID_ARGUMENT, "handle not provided");
    }

    std::unique_lock<std::mutex> lock(handles_mtx);
    auto id = request->handle().handle();
    if (!handles.contains(id)) {
        return Status(NOT_FOUND, "handle not exists");
    }
    auto handle = handles[id];
    lock.unlock();

    response->mutable_id()->set_id(MaaResourcePostPath(handle, request->str().c_str()));

    return Status::OK;
}

Status ResourceImpl::status(ServerContext* context, const ::maarpc::HandleIdRequest* request,
                            ::maarpc::StatusResponse* response)
{
    std::ignore = context;

    if (!request->has_handle()) {
        return Status(INVALID_ARGUMENT, "handle not provided");
    }
    if (!request->has_id()) {
        return Status(INVALID_ARGUMENT, "id not provided");
    }

    std::unique_lock<std::mutex> lock(handles_mtx);
    auto id = request->handle().handle();
    if (!handles.contains(id)) {
        return Status(NOT_FOUND, "handle not exists");
    }
    auto handle = handles[id];
    lock.unlock();

    response->set_value(MaaResourceStatus(handle, request->id().id()));

    return Status::OK;
}

Status ResourceImpl::wait(ServerContext* context, const ::maarpc::HandleIdRequest* request,
                          ::maarpc::StatusResponse* response)
{
    std::ignore = context;

    if (!request->has_handle()) {
        return Status(INVALID_ARGUMENT, "handle not provided");
    }
    if (!request->has_id()) {
        return Status(INVALID_ARGUMENT, "id not provided");
    }

    std::unique_lock<std::mutex> lock(handles_mtx);
    auto id = request->handle().handle();
    if (!handles.contains(id)) {
        return Status(NOT_FOUND, "handle not exists");
    }
    auto handle = handles[id];
    lock.unlock();

    response->set_value(MaaResourceWait(handle, request->id().id()));

    return Status::OK;
}

Status ResourceImpl::loaded(ServerContext* context, const ::maarpc::HandleRequest* request,
                            ::maarpc::BoolResponse* response)
{
    std::ignore = context;

    if (!request->has_handle()) {
        return Status(INVALID_ARGUMENT, "handle not provided");
    }

    std::unique_lock<std::mutex> lock(handles_mtx);
    auto id = request->handle().handle();
    if (!handles.contains(id)) {
        return Status(NOT_FOUND, "handle not exists");
    }
    auto handle = handles[id];
    lock.unlock();

    response->set_value(MaaResourceLoaded(handle));

    return Status::OK;
}

Status ResourceImpl::hash(ServerContext* context, const ::maarpc::HandleRequest* request,
                          ::maarpc::StringResponse* response)
{
    std::ignore = context;

    if (!request->has_handle()) {
        return Status(INVALID_ARGUMENT, "handle not provided");
    }

    std::unique_lock<std::mutex> lock(handles_mtx);
    auto id = request->handle().handle();
    if (!handles.contains(id)) {
        return Status(NOT_FOUND, "handle not exists");
    }
    auto handle = handles[id];
    lock.unlock();

    auto sb = MaaCreateStringBuffer();
    if (MaaResourceGetHash(handle, sb)) {
        std::string hash(MaaGetString(sb), MaaGetStringSize(sb));
        response->set_value(hash);
        return Status::OK;
    }
    else {
        return Status(UNKNOWN, "MaaResourceGetHash failed");
    }
}
