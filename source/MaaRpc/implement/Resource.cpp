#include "Resource.h"
#include "MaaFramework/MaaAPI.h"
#include "Macro.h"
#include "Utils/Logger.h"

using namespace ::grpc;

Status ResourceImpl::create(ServerContext* context, const ::maarpc::IdRequest* request,
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
    handles_.add(id, MaaResourceCreate(callback_impl, cb_state.get()));

    response->set_handle(id);

    return Status::OK;
}

Status ResourceImpl::destroy(ServerContext* context, const ::maarpc::HandleRequest* request,
                             ::maarpc::EmptyResponse* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_DEL_HANDLE

    MaaResourceDestroy(handle);

    return Status::OK;
}

Status ResourceImpl::post_path(ServerContext* context, const ::maarpc::HandleStringRequest* request,
                               ::maarpc::IIdResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(str)

    MAA_GRPC_GET_HANDLE

    response->set_id(MaaResourcePostPath(handle, request->str().c_str()));

    return Status::OK;
}

Status ResourceImpl::status(ServerContext* context, const ::maarpc::HandleIIdRequest* request,
                            ::maarpc::StatusResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(id)

    MAA_GRPC_GET_HANDLE

    response->set_status(MaaResourceStatus(handle, request->id()));

    return Status::OK;
}

Status ResourceImpl::wait(ServerContext* context, const ::maarpc::HandleIIdRequest* request,
                          ::maarpc::StatusResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(id)

    MAA_GRPC_GET_HANDLE

    response->set_status(MaaResourceWait(handle, request->id()));

    return Status::OK;
}

Status ResourceImpl::loaded(ServerContext* context, const ::maarpc::HandleRequest* request,
                            ::maarpc::BoolResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    response->set_bool_(MaaResourceLoaded(handle));

    return Status::OK;
}

Status ResourceImpl::hash(ServerContext* context, const ::maarpc::HandleRequest* request,
                          ::maarpc::StringResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

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

Status ResourceImpl::task_list(ServerContext* context, const ::maarpc::HandleRequest* request,
                               ::maarpc::StringResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    auto sb = MaaCreateStringBuffer();
    if (MaaResourceGetTaskList(handle, sb)) {
        std::string list(MaaGetString(sb), MaaGetStringSize(sb));
        MaaDestroyStringBuffer(sb);
        response->set_str(list);
        return Status::OK;
    }
    else {
        MaaDestroyStringBuffer(sb);
        return Status(UNKNOWN, "MaaResourceGetTaskList failed");
    }
}
