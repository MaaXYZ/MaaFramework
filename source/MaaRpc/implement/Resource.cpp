#include "Resource.h"
#include "MaaFramework/MaaAPI.h"
#include "macro.h"

using namespace ::grpc;

Status ResourceImpl::create(ServerContext* context, const ::maarpc::IdRequest* request,
                            ::maarpc::HandleResponse* response)
{
    std::ignore = context;

    MAA_GRPC_REQUIRED(id)

    auto cbId = request->id();
    auto cbState = uImpl->get(cbId);
    if (!cbState) {
        return Status(NOT_FOUND, "id not exists");
    }

    auto id = make_uuid();
    auto handle = MaaResourceCreate(CallbackImpl, cbState);

    std::unique_lock<std::mutex> lock(handles_mtx);
    handles[id] = handle;
    lock.unlock();

    response->set_handle(id);

    return Status::OK;
}

Status ResourceImpl::destroy(ServerContext* context, const ::maarpc::HandleRequest* request,
                             ::maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)

    std::unique_lock<std::mutex> lock(handles_mtx);
    auto id = request->handle();
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
                               ::maarpc::IIdResponse* response)
{
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)

    std::unique_lock<std::mutex> lock(handles_mtx);
    auto id = request->handle();
    if (!handles.contains(id)) {
        return Status(NOT_FOUND, "handle not exists");
    }
    auto handle = handles[id];
    lock.unlock();

    response->set_id(MaaResourcePostPath(handle, request->str().c_str()));

    return Status::OK;
}

Status ResourceImpl::status(ServerContext* context, const ::maarpc::HandleIIdRequest* request,
                            ::maarpc::StatusResponse* response)
{
    std::ignore = context;

    MAA_GRPC_REQUIRED(id)
    MAA_GRPC_REQUIRED(handle)

    std::unique_lock<std::mutex> lock(handles_mtx);
    auto id = request->handle();
    if (!handles.contains(id)) {
        return Status(NOT_FOUND, "handle not exists");
    }
    auto handle = handles[id];
    lock.unlock();

    response->set_status(MaaResourceStatus(handle, request->id()));

    return Status::OK;
}

Status ResourceImpl::wait(ServerContext* context, const ::maarpc::HandleIIdRequest* request,
                          ::maarpc::StatusResponse* response)
{
    std::ignore = context;

    MAA_GRPC_REQUIRED(id)
    MAA_GRPC_REQUIRED(handle)

    std::unique_lock<std::mutex> lock(handles_mtx);
    auto id = request->handle();
    if (!handles.contains(id)) {
        return Status(NOT_FOUND, "handle not exists");
    }
    auto handle = handles[id];
    lock.unlock();

    response->set_status(MaaResourceWait(handle, request->id()));

    return Status::OK;
}

Status ResourceImpl::loaded(ServerContext* context, const ::maarpc::HandleRequest* request,
                            ::maarpc::BoolResponse* response)
{
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)

    std::unique_lock<std::mutex> lock(handles_mtx);
    auto id = request->handle();
    if (!handles.contains(id)) {
        return Status(NOT_FOUND, "handle not exists");
    }
    auto handle = handles[id];
    lock.unlock();

    response->set_bool_(MaaResourceLoaded(handle));

    return Status::OK;
}

Status ResourceImpl::hash(ServerContext* context, const ::maarpc::HandleRequest* request,
                          ::maarpc::StringResponse* response)
{
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)

    std::unique_lock<std::mutex> lock(handles_mtx);
    auto id = request->handle();
    if (!handles.contains(id)) {
        return Status(NOT_FOUND, "handle not exists");
    }
    auto handle = handles[id];
    lock.unlock();

    auto sb = MaaCreateStringBuffer();
    if (MaaResourceGetHash(handle, sb)) {
        std::string hash(MaaGetString(sb), MaaGetStringSize(sb));
        MaaDestroyStringBuffer(sb);
        response->set_str(hash);
        return Status::OK;
    }
    else {
        MaaDestroyStringBuffer(sb);
        return Status(UNKNOWN, "MaaResourceGetHash failed");
    }
}
