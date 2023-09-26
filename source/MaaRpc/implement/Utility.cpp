#include "Utility.h"
#include "MaaFramework/MaaAPI.h"

using namespace ::grpc;

void CallbackImpl(MaaStringView msg, MaaStringView detail, MaaCallbackTransparentArg arg)
{
    ::maarpc::Callback cb;
    cb.set_msg(msg);
    cb.set_detail(detail);

    auto* state = reinterpret_cast<UtilityImpl::CallbackState*>(arg);
    state->write.acquire();
    state->writer->Write(cb);
    state->write.release();
}

UtilityImpl::CallbackState* UtilityImpl::get(uint64_t id)
{
    std::lock_guard<std::mutex> lock(state_mtx);
    if (states.contains(id)) {
        return states[id];
    }
    else {
        return nullptr;
    }
}

Status UtilityImpl::version(ServerContext* context, const ::maarpc::EmptyRequest* request,
                            ::maarpc::StringResponse* response)
{
    std::ignore = context;
    std::ignore = request;

    response->set_value(MaaVersion());

    return Status::OK;
}

Status UtilityImpl::set_global_option(ServerContext* context, const ::maarpc::SetGlobalOptionRequest* request,
                                      ::maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = response;

    switch (request->option_case()) {
    case ::maarpc::SetGlobalOptionRequest::OptionCase::kLogging:
        if (request->has_logging()) {
            auto logging = request->logging();
            if (MaaSetGlobalOption(MaaGlobalOption_Logging, const_cast<char*>(logging.c_str()), logging.size())) {
                return Status::OK;
            }
            else {
                return Status(UNKNOWN, "MaaSetGlobalOption failed");
            }
        }
        break;
    case ::maarpc::SetGlobalOptionRequest::OptionCase::kDebugMode:
        if (request->has_debug_mode()) {
            MaaBool mode = request->debug_mode() ? 1 : 0;
            if (MaaSetGlobalOption(MaaGlobalOption_DebugMode, &mode, 1)) {
                return Status::OK;
            }
            else {
                return Status(UNKNOWN, "MaaSetGlobalOption failed");
            }
        }
        break;
    default:
        break;
    }
    return Status(ABORTED, "protobuf `oneof` state invalid");
}

Status UtilityImpl::acquire_callback_id(ServerContext* context, const ::maarpc::EmptyRequest* request,
                                        ::maarpc::IdResponse* response)
{
    std::ignore = context;
    std::ignore = request;

    auto id = callback_id_counter++;
    response->mutable_id()->set_id(id);

    return Status::OK;
}

Status UtilityImpl::register_callback(ServerContext* context, const ::maarpc::IdRequest* request,
                                      ServerWriter<::maarpc::Callback>* writer)
{
    std::ignore = context;

    if (!request->has_id()) {
        return Status(INVALID_ARGUMENT, "id not provided");
    }

    auto id = request->id().id();

    std::unique_lock<std::mutex> lock(state_mtx);
    if (states.contains(id)) {
        return Status(ALREADY_EXISTS, "id already registered");
    }

    auto& pstate = states[id];
    pstate = new CallbackState;

    auto& state = *pstate;
    state.writer = writer;
    lock.unlock();

    state.finish.acquire();
    state.write.acquire(); // 等待callback完成

    delete &state; // pstate此时已经被erase了

    return Status::OK;
}

Status UtilityImpl::unregister_callback(ServerContext* context, const ::maarpc::IdRequest* request,
                                        ::maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = response;

    if (!request->has_id()) {
        return Status(INVALID_ARGUMENT, "id not provided");
    }

    auto id = request->id().id();

    std::unique_lock<std::mutex> lock(state_mtx);
    if (!states.contains(id)) {
        return Status(NOT_FOUND, "id not exists");
    }

    auto pstate = states[id];
    states.erase(id);

    lock.unlock();

    pstate->finish.release();

    return Status::OK;
}

Status UtilityImpl::acquire_custom_controller_id(ServerContext* context, const ::maarpc::EmptyRequest* request,
                                                 ::maarpc::IdResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return Status(UNIMPLEMENTED, "");
}
