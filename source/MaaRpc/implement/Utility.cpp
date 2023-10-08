#include "Utility.h"
#include "MaaFramework/MaaAPI.h"
#include "macro.h"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

using namespace ::grpc;

auto uuid_generator = boost::uuids::random_generator();
std::string make_uuid()
{
    return boost::uuids::to_string(uuid_generator());
}

void callback_impl(MaaStringView msg, MaaStringView detail, MaaCallbackTransparentArg arg)
{
    ::maarpc::Callback cb;
    cb.set_msg(msg);
    cb.set_detail(detail);

    auto* state = reinterpret_cast<UtilityImpl::CallbackState*>(arg);
    state->write.acquire();
    state->writer->Write(cb);
    state->write.release();
}

Status UtilityImpl::version(ServerContext* context, const ::maarpc::EmptyRequest* request,
                            ::maarpc::StringResponse* response)
{
    std::ignore = context;
    std::ignore = request;

    response->set_str(MaaVersion());

    return Status::OK;
}

Status UtilityImpl::set_global_option(ServerContext* context, const ::maarpc::SetGlobalOptionRequest* request,
                                      ::maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED_CASE(option, OPTION)

    switch (request->option_case()) {
    case ::maarpc::SetGlobalOptionRequest::OptionCase::kLogging:
        if (request->has_logging()) {
            auto logging = request->logging();
            if (MaaSetGlobalOption(MaaGlobalOption_Logging, logging.data(), logging.size())) {
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

Status UtilityImpl::acquire_id(ServerContext* context, const ::maarpc::EmptyRequest* request,
                               ::maarpc::IdResponse* response)
{
    std::ignore = context;
    std::ignore = request;

    response->set_id(make_uuid());

    return Status::OK;
}

Status UtilityImpl::register_callback(ServerContext* context, const ::maarpc::IdRequest* request,
                                      ServerWriter<::maarpc::Callback>* writer)
{
    std::ignore = context;

    MAA_GRPC_REQUIRED(id)

    auto id = request->id();

    if (states_.has(id)) {
        return Status(ALREADY_EXISTS, "id already registered");
    }

    auto state = std::make_shared<CallbackState>();
    state->writer = writer;

    states_.add(id, state);

    state->finish.acquire();
    state->write.acquire(); // 等待callback完成

    return Status::OK;
}

Status UtilityImpl::unregister_callback(ServerContext* context, const ::maarpc::IdRequest* request,
                                        ::maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(id)

    auto id = request->id();

    std::shared_ptr<CallbackState> state = nullptr;
    if (!states_.del(id, state)) {
        return Status(NOT_FOUND, "id not exists");
    }

    state->finish.release();

    return Status::OK;
}
