#include "Utility.h"
#include "MaaFramework/MaaAPI.h"
#include "Macro.h"
#include "Utils/Logger.h"

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
    state->stream->Write(cb);

    ::maarpc::CallbackRequest br;
    state->stream->Read(&br);
    state->write.release();
}

Status UtilityImpl::version(ServerContext* context, const ::maarpc::EmptyRequest* request,
                            ::maarpc::StringResponse* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = request;

    response->set_str(MaaVersion());

    return Status::OK;
}

Status UtilityImpl::set_global_option(ServerContext* context, const ::maarpc::SetGlobalOptionRequest* request,
                                      ::maarpc::EmptyResponse* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED_CASE(option, OPTION)

    switch (request->option_case()) {
    case ::maarpc::SetGlobalOptionRequest::OptionCase::kLogDir:
        if (request->has_log_dir()) {
            std::string log_dir = request->log_dir();
            if (MaaSetGlobalOption(MaaGlobalOption_LogDir, log_dir.data(), log_dir.size())) {
                return Status::OK;
            }
            else {
                return Status(UNKNOWN, "MaaSetGlobalOption failed");
            }
        }
        break;
    case ::maarpc::SetGlobalOptionRequest::OptionCase::kSaveDraw:
        if (request->has_save_draw()) {
            bool mode = request->save_draw();
            if (MaaSetGlobalOption(MaaGlobalOption_SaveDraw, &mode, sizeof(mode))) {
                return Status::OK;
            }
            else {
                return Status(UNKNOWN, "MaaSetGlobalOption failed");
            }
        }
        break;
    case ::maarpc::SetGlobalOptionRequest::OptionCase::kRecording:
        if (request->has_recording()) {
            bool mode = request->recording();
            if (MaaSetGlobalOption(MaaGlobalOption_Recording, &mode, sizeof(mode))) {
                return Status::OK;
            }
            else {
                return Status(UNKNOWN, "MaaSetGlobalOption failed");
            }
        }
        break;
    case ::maarpc::SetGlobalOptionRequest::OptionCase::kStdoutLevel:
        if (request->has_stdout_level()) {
            int32_t level = request->stdout_level();
            if (MaaSetGlobalOption(MaaGlobalOption_StdoutLevel, &level, sizeof(level))) {
                return Status::OK;
            }
            else {
                return Status(UNKNOWN, "MaaSetGlobalOption failed");
            }
        }
        break;
    case ::maarpc::SetGlobalOptionRequest::OptionCase::kShowDraw:
        if (request->has_show_draw()) {
            bool mode = request->show_draw();
            if (MaaSetGlobalOption(MaaGlobalOption_ShowDraw, &mode, sizeof(mode))) {
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
    LogFunc;
    std::ignore = context;
    std::ignore = request;

    response->set_id(make_uuid());

    return Status::OK;
}

Status UtilityImpl::register_callback(ServerContext* context,
                                      ServerReaderWriter<::maarpc::Callback, ::maarpc::CallbackRequest>* stream)
{
    LogFunc;

    ::maarpc::CallbackRequest request_data;
    auto request = &request_data;

    if (!stream->Read(request)) {
        return Status(FAILED_PRECONDITION, "register callback cannot read init");
    }

    MAA_GRPC_REQUIRED_CASE_AS(result, Init)

    MAA_GRPC_REQUIRED_OF(id, (&request->init()))

    auto id = request->init().id();

    if (states_.has(id)) {
        return Status(ALREADY_EXISTS, "id already registered");
    }

    auto state = std::make_shared<CallbackState>();
    state->stream = stream;

    states_.add(id, state);

    {
        ::maarpc::Callback cb;
        cb.set_msg("Rpc.Inited");
        state->stream->Write(cb);
    }

    while (true) {
        using namespace std::chrono_literals;
        if (state->finish.try_acquire_for(2s)) {
            break;
        }
        if (context->IsCancelled()) {
            return Status::CANCELLED;
        }
    }
    // 等待callback完成
    state->write.acquire();

    return Status::OK;
}

Status UtilityImpl::unregister_callback(ServerContext* context, const ::maarpc::IdRequest* request,
                                        ::maarpc::EmptyResponse* response)
{
    LogFunc;
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
