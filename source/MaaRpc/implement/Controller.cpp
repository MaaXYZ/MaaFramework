#include "Controller.h"
#include "MaaFramework/MaaAPI.h"
#include "Macro.h"
#include "Utils/Logger.h"

using namespace ::grpc;

Status ControllerImpl::create_adb(::grpc::ServerContext* context, const ::maarpc::AdbControllerRequest* request,
                                  ::maarpc::HandleResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(id)
    MAA_GRPC_REQUIRED(adb_path)
    MAA_GRPC_REQUIRED(adb_serial)
    MAA_GRPC_REQUIRED(adb_type)
    MAA_GRPC_REQUIRED(adb_config)

    auto cb_id = request->id();
    std::shared_ptr<UtilityImpl::CallbackState> cb_state = nullptr;
    if (!utility_impl_->states().get(cb_id, cb_state)) {
        return Status(NOT_FOUND, "id not exists");
    }

    auto id = make_uuid();

    if (request->has_agent_path()) {
        handles_.add(id, MaaAdbControllerCreateV2(request->adb_path().c_str(), request->adb_serial().c_str(),
                                                  request->adb_type(), request->adb_config().c_str(),
                                                  request->agent_path().c_str(), callback_impl, cb_state.get()));
    }
    else {
        handles_.add(id, MaaAdbControllerCreate(request->adb_path().c_str(), request->adb_serial().c_str(),
                                                request->adb_type(), request->adb_config().c_str(), callback_impl,
                                                cb_state.get()));
    }

    response->set_handle(id);

    return Status::OK;
}

Status ControllerImpl::destroy(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                               ::maarpc::EmptyResponse* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_DEL_HANDLE

    MaaControllerDestroy(handle);

    std::shared_ptr<CustomControllerInfo> info = nullptr;
    if (infos_.del(request->handle(), info)) {
        info->finish.release();
    }

    return Status::OK;
}

static MaaBool _connect(MaaTransparentArg arg)
{
    auto info = reinterpret_cast<ControllerImpl::CustomControllerInfo*>(arg);
    auto stream = info->stream;

    ::maarpc::CustomControllerResponse response;
    response.set_connect(true);
    stream->Write(response);

    ::maarpc::CustomControllerRequest request;
    stream->Read(&request);

    return request.ok();
}

static MaaBool _click(int32_t x, int32_t y, MaaTransparentArg arg)
{
    auto info = reinterpret_cast<ControllerImpl::CustomControllerInfo*>(arg);
    auto stream = info->stream;

    ::maarpc::CustomControllerResponse response;
    response.mutable_click()->mutable_point()->set_x(x);
    response.mutable_click()->mutable_point()->set_x(y);
    stream->Write(response);

    ::maarpc::CustomControllerRequest request;
    stream->Read(&request);

    return request.ok();
}

static MaaBool _swipe(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t duration, MaaTransparentArg arg)
{
    auto info = reinterpret_cast<ControllerImpl::CustomControllerInfo*>(arg);
    auto stream = info->stream;

    ::maarpc::CustomControllerResponse response;
    response.mutable_swipe()->mutable_from()->set_x(x1);
    response.mutable_swipe()->mutable_from()->set_x(y1);
    response.mutable_swipe()->mutable_to()->set_x(x2);
    response.mutable_swipe()->mutable_to()->set_x(y2);
    response.mutable_swipe()->set_duration(duration);
    stream->Write(response);

    ::maarpc::CustomControllerRequest request;
    stream->Read(&request);

    return request.ok();
}

static MaaBool _press_key(int32_t key, MaaTransparentArg arg)
{
    auto info = reinterpret_cast<ControllerImpl::CustomControllerInfo*>(arg);
    auto stream = info->stream;

    ::maarpc::CustomControllerResponse response;
    response.mutable_press_key()->set_key(key);
    stream->Write(response);

    ::maarpc::CustomControllerRequest request;
    stream->Read(&request);

    return request.ok();
}

static MaaBool _touch_down(int32_t contact, int32_t x, int32_t y, int32_t pressure, MaaTransparentArg arg)
{
    auto info = reinterpret_cast<ControllerImpl::CustomControllerInfo*>(arg);
    auto stream = info->stream;

    ::maarpc::CustomControllerResponse response;
    response.mutable_touch_down()->set_contact(contact);
    response.mutable_touch_down()->mutable_pos()->set_x(x);
    response.mutable_touch_down()->mutable_pos()->set_y(y);
    response.mutable_touch_down()->set_pressure(pressure);
    stream->Write(response);

    ::maarpc::CustomControllerRequest request;
    stream->Read(&request);

    return request.ok();
}

static MaaBool _touch_move(int32_t contact, int32_t x, int32_t y, int32_t pressure, MaaTransparentArg arg)
{
    auto info = reinterpret_cast<ControllerImpl::CustomControllerInfo*>(arg);
    auto stream = info->stream;

    ::maarpc::CustomControllerResponse response;
    response.mutable_touch_move()->set_contact(contact);
    response.mutable_touch_move()->mutable_pos()->set_x(x);
    response.mutable_touch_move()->mutable_pos()->set_y(y);
    response.mutable_touch_move()->set_pressure(pressure);
    stream->Write(response);

    ::maarpc::CustomControllerRequest request;
    stream->Read(&request);

    return request.ok();
}

static MaaBool _touch_up(int32_t contact, MaaTransparentArg arg)
{
    auto info = reinterpret_cast<ControllerImpl::CustomControllerInfo*>(arg);
    auto stream = info->stream;

    ::maarpc::CustomControllerResponse response;
    response.mutable_touch_up()->set_contact(contact);
    stream->Write(response);

    ::maarpc::CustomControllerRequest request;
    stream->Read(&request);

    return request.ok();
}

static MaaBool _start_app(MaaStringView entry, MaaTransparentArg arg)
{
    auto info = reinterpret_cast<ControllerImpl::CustomControllerInfo*>(arg);
    auto stream = info->stream;

    ::maarpc::CustomControllerResponse response;
    response.set_start_app(entry);
    stream->Write(response);

    ::maarpc::CustomControllerRequest request;
    stream->Read(&request);

    return request.ok();
}

static MaaBool _stop_app(MaaStringView entry, MaaTransparentArg arg)
{
    auto info = reinterpret_cast<ControllerImpl::CustomControllerInfo*>(arg);
    auto stream = info->stream;

    ::maarpc::CustomControllerResponse response;
    response.set_stop_app(entry);
    stream->Write(response);

    ::maarpc::CustomControllerRequest request;
    stream->Read(&request);

    return request.ok();
}

static MaaBool _request_resolution(MaaTransparentArg arg, int32_t* width, int32_t* height)
{
    auto info = reinterpret_cast<ControllerImpl::CustomControllerInfo*>(arg);
    auto stream = info->stream;

    ::maarpc::CustomControllerResponse response;
    response.set_request_resolution(true);
    stream->Write(response);

    ::maarpc::CustomControllerRequest request;
    stream->Read(&request);

    if (request.result_case() != ::maarpc::CustomControllerRequest::kResolution) {
        return false;
    }

    *width = request.resolution().width();
    *height = request.resolution().height();

    return request.ok();
}

static MaaBool _screencap(MaaTransparentArg arg, MaaImageBufferHandle buffer)
{
    auto info = reinterpret_cast<ControllerImpl::CustomControllerInfo*>(arg);
    auto stream = info->stream;

    ::maarpc::CustomControllerResponse response;

    auto id = make_uuid();
    info->image_impl->handles().add(id, buffer);
    response.set_screencap(id);

    stream->Write(response);

    ::maarpc::CustomControllerRequest request;
    stream->Read(&request);

    info->image_impl->handles().del(id);

    return request.ok();
}

static MaaBool _request_uuid(MaaTransparentArg arg, MaaStringBufferHandle buffer)
{
    auto info = reinterpret_cast<ControllerImpl::CustomControllerInfo*>(arg);
    auto stream = info->stream;

    ::maarpc::CustomControllerResponse response;
    response.set_request_uuid(true);
    stream->Write(response);

    ::maarpc::CustomControllerRequest request;
    stream->Read(&request);

    if (request.result_case() != ::maarpc::CustomControllerRequest::kUuid) {
        return false;
    }

    const auto& str = request.uuid();
    MaaSetStringEx(buffer, str.c_str(), str.size());

    return request.ok();
}

static MaaCustomControllerAPI custom_controller_api = {
    .connect = _connect,
    .request_uuid = _request_uuid,
    .request_resolution = _request_resolution,
    .start_app = _start_app,
    .stop_app = _stop_app,
    .screencap = _screencap,
    .click = _click,
    .swipe = _swipe,
    .touch_down = _touch_down,
    .touch_move = _touch_move,
    .touch_up = _touch_up,
    .press_key = _press_key,
};

Status ControllerImpl::create_custom(
    ServerContext* context,
    ServerReaderWriter<::maarpc::CustomControllerResponse, ::maarpc::CustomControllerRequest>* stream)
{
    LogFunc;
    std::ignore = context;

    ::maarpc::CustomControllerRequest request_data;
    auto request = &request_data;

    if (!stream->Read(request)) {
        return Status(FAILED_PRECONDITION, "custom controller cannot read init");
    }

    MAA_GRPC_REQUIRED_CASE_AS(result, Init)

    auto cbId = request->init();
    std::shared_ptr<UtilityImpl::CallbackState> cb_state = nullptr;
    if (!utility_impl_->states().get(cbId, cb_state)) {
        return Status(NOT_FOUND, "id not exists");
    }

#if defined(__APPLE__) && defined(__clang__) && __clang_major__ < 16
    std::shared_ptr<CustomControllerInfo> info(new CustomControllerInfo { stream, image_impl_ });
#else
    // build error of Apple clang version 15.0.0 (clang-1500.0.40.1) on macOS
    auto info = std::make_shared<CustomControllerInfo>(stream, image_impl_);
#endif

    auto id = make_uuid();

    ::maarpc::CustomControllerResponse response;
    response.set_init(id);
    stream->Write(response);

    handles_.add(id, MaaCustomControllerCreate(&custom_controller_api, &info, callback_impl, cb_state.get()));
    infos_.add(id, info);

    info->finish.acquire();

    return Status::OK;
}

Status ControllerImpl::set_option(::grpc::ServerContext* context, const ::maarpc::ControllerSetOptionRequest* request,
                                  ::maarpc::EmptyResponse* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED_CASE(option, OPTION)

    MAA_GRPC_GET_HANDLE

    switch (request->option_case()) {
    case ::maarpc::ControllerSetOptionRequest::kLongSide:
        if (request->has_long_side()) {
            auto side = request->long_side();
            if (MaaControllerSetOption(handle, MaaCtrlOption_ScreenshotTargetLongSide, &side, 4)) {
                return Status::OK;
            }
            else {
                return Status(UNKNOWN, "MaaControllerSetOption failed");
            }
        }
        break;
    case ::maarpc::ControllerSetOptionRequest::kShortSide:
        if (request->has_short_side()) {
            auto side = request->short_side();
            if (MaaControllerSetOption(handle, MaaCtrlOption_ScreenshotTargetShortSide, &side, 4)) {
                return Status::OK;
            }
            else {
                return Status(UNKNOWN, "MaaControllerSetOption failed");
            }
        }
        break;
    case ::maarpc::ControllerSetOptionRequest::kDefPackageEntry:
        if (request->has_def_package_entry()) {
            auto entry = request->def_package_entry();
            if (MaaControllerSetOption(handle, MaaCtrlOption_DefaultAppPackageEntry, entry.data(), entry.size())) {
                return Status::OK;
            }
            else {
                return Status(UNKNOWN, "MaaControllerSetOption failed");
            }
        }
        break;
    case ::maarpc::ControllerSetOptionRequest::kDefPackage:
        if (request->has_def_package()) {
            auto entry = request->def_package();
            if (MaaControllerSetOption(handle, MaaCtrlOption_DefaultAppPackage, entry.data(), entry.size())) {
                return Status::OK;
            }
            else {
                return Status(UNKNOWN, "MaaControllerSetOption failed");
            }
        }
        break;
    default:
        break;
    }

    return Status(ABORTED, "protobuf `oneof` state invalid");
}

Status ControllerImpl::post_connection(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                                       ::maarpc::IIdResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    response->set_id(MaaControllerPostConnection(handle));

    return Status::OK;
}

Status ControllerImpl::post_click(::grpc::ServerContext* context, const ::maarpc::ControllerPostClickRequest* request,
                                  ::maarpc::IIdResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(param)

    const auto& param = request->param();

    MAA_GRPC_GET_HANDLE

    response->set_id(MaaControllerPostClick(handle, param.point().x(), param.point().y()));

    return Status::OK;
}

Status ControllerImpl::post_swipe(::grpc::ServerContext* context, const ::maarpc::ControllerPostSwipeRequest* request,
                                  ::maarpc::IIdResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(param)

    const auto& param = request->param();

    MAA_GRPC_GET_HANDLE

    response->set_id(MaaControllerPostSwipe(handle, param.from().x(), param.from().y(), param.to().x(), param.to().y(),
                                            param.duration()));

    return Status::OK;
}

Status ControllerImpl::post_press_key(::grpc::ServerContext* context, const ::maarpc::ControllerPostKeyRequest* request,
                                      ::maarpc::IIdResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(param)

    const auto& param = request->param();

    MAA_GRPC_GET_HANDLE

    response->set_id(MaaControllerPostPressKey(handle, param.key()));

    return Status::OK;
}

Status ControllerImpl::post_touch_down(::grpc::ServerContext* context,
                                       const ::maarpc::ControllerPostTouchRequest* request,
                                       ::maarpc::IIdResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(param)

    const auto& param = request->param();

    MAA_GRPC_GET_HANDLE

    response->set_id(
        MaaControllerPostTouchDown(handle, param.contact(), param.pos().x(), param.pos().y(), param.pressure()));

    return Status::OK;
}

Status ControllerImpl::post_touch_move(::grpc::ServerContext* context,
                                       const ::maarpc::ControllerPostTouchRequest* request,
                                       ::maarpc::IIdResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(param)

    const auto& param = request->param();

    MAA_GRPC_GET_HANDLE

    response->set_id(
        MaaControllerPostTouchMove(handle, param.contact(), param.pos().x(), param.pos().y(), param.pressure()));

    return Status::OK;
}

Status ControllerImpl::post_touch_up(::grpc::ServerContext* context,
                                     const ::maarpc::ControllerPostTouchRequest* request,
                                     ::maarpc::IIdResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(param)

    const auto& param = request->param();

    MAA_GRPC_GET_HANDLE

    response->set_id(MaaControllerPostTouchUp(handle, param.contact()));

    return Status::OK;
}

Status ControllerImpl::post_screencap(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                                      ::maarpc::IIdResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    response->set_id(MaaControllerPostScreencap(handle));

    return Status::OK;
}

Status ControllerImpl::status(::grpc::ServerContext* context, const ::maarpc::HandleIIdRequest* request,
                              ::maarpc::StatusResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(id)

    MAA_GRPC_GET_HANDLE

    response->set_status(MaaControllerStatus(handle, request->id()));

    return Status::OK;
}

Status ControllerImpl::wait(::grpc::ServerContext* context, const ::maarpc::HandleIIdRequest* request,
                            ::maarpc::StatusResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(id)

    MAA_GRPC_GET_HANDLE

    response->set_status(MaaControllerWait(handle, request->id()));

    return Status::OK;
}

Status ControllerImpl::connected(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                                 ::maarpc::BoolResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    response->set_bool_(MaaControllerConnected(handle));

    return Status::OK;
}

Status ControllerImpl::image(::grpc::ServerContext* context, const ::maarpc::HandleHandleRequest* request,
                             ::maarpc::EmptyResponse* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(another_handle)

    MAA_GRPC_GET_HANDLE
    MAA_GRPC_GET_HANDLE_FROM(image_impl_, image_handle, another_handle)

    if (MaaControllerGetImage(handle, image_handle)) {
        return Status::OK;
    }
    else {
        return Status(UNKNOWN, "MaaControllerGetImage failed");
    }
}

Status ControllerImpl::uuid(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                            ::maarpc::StringResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    auto sb = MaaCreateStringBuffer();
    if (MaaControllerGetUUID(handle, sb)) {
        std::string uuid(MaaGetString(sb), MaaGetStringSize(sb));
        MaaDestroyStringBuffer(sb);
        response->set_str(uuid);
        return Status::OK;
    }
    else {
        MaaDestroyStringBuffer(sb);
        return Status(UNKNOWN, "MaaControllerGetUUID failed");
    }
}
