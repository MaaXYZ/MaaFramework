#include "MaaFrameworkImpl.h"
#include "MaaFramework/MaaAPI.h"

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
    std::ignore = response;
    return grpc::Status::CANCELLED;
}

grpc::Status MaaFrameworkImpl::register_callback(grpc::ServerContext* context, const maarpc::IdRequest* request,
                                                 grpc::ServerWriter<maarpc::Callback>* writer)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = writer;
    return grpc::Status::CANCELLED;
}

grpc::Status MaaFrameworkImpl::unregister_callback(grpc::ServerContext* context, const maarpc::IdRequest* request,
                                                   maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;
    return grpc::Status::CANCELLED;
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