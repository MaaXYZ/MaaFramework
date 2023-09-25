#include "MaaFrameworkImpl.h"
#include "MaaFramework/MaaAPI.h"

void CallbackImpl(MaaStringView msg, MaaStringView detail, MaaCallbackTransparentArg arg)
{
    maarpc::Callback cb;
    cb.set_msg(msg);
    cb.set_detail(detail);

    auto* state = reinterpret_cast<MaaFrameworkImpl::CallbackState*>(arg);
    state->write.acquire();
    state->writer->Write(cb);
    state->write.release();
}

MaaFrameworkImpl::CallbackState* MaaFrameworkImpl::get(uint64_t id)
{
    std::lock_guard<std::mutex> lock(state_mtx);
    if (states.contains(id)) {
        return states[id];
    }
    else {
        return nullptr;
    }
}

grpc::Status MaaFrameworkImpl::version(grpc::ServerContext* context, const maarpc::EmptyRequest* request,
                                       maarpc::StringResponse* response)
{
    std::ignore = context;
    std::ignore = request;

    response->set_status(true);
    response->set_value(MaaVersion());
    return grpc::Status::OK;
}

grpc::Status MaaFrameworkImpl::set_global_option(grpc::ServerContext* context,
                                                 const maarpc::SetGlobalOptionRequest* request,
                                                 maarpc::EmptyResponse* response)
{
    std::ignore = context;

    switch (request->option_case()) {
    case maarpc::SetGlobalOptionRequest::OptionCase::kLogging:
        if (request->has_logging()) {
            auto logging = request->logging();
            if (MaaSetGlobalOption(MaaGlobalOption_Logging, const_cast<char*>(logging.c_str()), logging.size())) {
                response->set_status(true);
                return grpc::Status::OK;
            }
        }
        response->set_status(false);
        return grpc::Status::OK;
    case maarpc::SetGlobalOptionRequest::OptionCase::kDebugMode:
        if (request->has_debug_mode()) {
            MaaBool mode = request->debug_mode() ? 1 : 0;
            if (MaaSetGlobalOption(MaaGlobalOption_DebugMode, &mode, 1)) {
                response->set_status(true);
                return grpc::Status::OK;
            }
        }
        response->set_status(false);
        return grpc::Status::OK;
    default:
        return grpc::Status::CANCELLED;
    }
}

grpc::Status MaaFrameworkImpl::acquire_callback_id(grpc::ServerContext* context, const maarpc::EmptyRequest* request,
                                                   maarpc::IdResponse* response)
{
    std::ignore = context;
    std::ignore = request;

    auto id = callback_id_counter++;
    response->mutable_id()->set_id(id);
    return grpc::Status::OK;
}

grpc::Status MaaFrameworkImpl::register_callback(grpc::ServerContext* context, const maarpc::IdRequest* request,
                                                 grpc::ServerWriter<maarpc::Callback>* writer)
{
    std::ignore = context;

    auto id = request->id().id();

    std::unique_lock<std::mutex> lock(state_mtx);
    if (states.contains(id)) {
        return grpc::Status::CANCELLED;
    }

    auto& pstate = states[id];
    pstate = new CallbackState;

    auto& state = *pstate;
    state.writer = writer;
    lock.unlock();

    state.finish.acquire();
    state.write.acquire(); // 等待callback完成

    delete &state; // pstate此时已经被erase了

    return grpc::Status::OK;
}

grpc::Status MaaFrameworkImpl::unregister_callback(grpc::ServerContext* context, const maarpc::IdRequest* request,
                                                   maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = response;

    auto id = request->id().id();

    std::unique_lock<std::mutex> lock(state_mtx);
    if (!states.contains(id)) {
        return grpc::Status::CANCELLED;
    }

    auto pstate = states[id];
    states.erase(id);

    lock.unlock();

    pstate->finish.release();

    return grpc::Status::OK;
}

grpc::Status MaaFrameworkImpl::acquire_custom_controller_id(grpc::ServerContext* context,
                                                            const maarpc::EmptyRequest* request,
                                                            maarpc::IdResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return grpc::Status::CANCELLED;
}
