#include "Controller.h"
#include "macro.h"

using namespace ::grpc;

Status ControllerImpl::create_adb(::grpc::ServerContext* context, const ::maarpc::AdbControllerRequest* request,
                                  ::maarpc::HandleResponse* response)
{
    std::ignore = context;

    MAA_GRPC_REQUIRED(id)
    MAA_GRPC_REQUIRED(adb_path)
    MAA_GRPC_REQUIRED(adb_serial)
    MAA_GRPC_REQUIRED(adb_type)
    MAA_GRPC_REQUIRED(adb_config)

    auto cbId = request->id();
    auto cbState = uImpl->get(cbId);
    if (!cbState) {
        return Status(NOT_FOUND, "id not exists");
    }

    auto id = make_uuid();
    auto handle = MaaAdbControllerCreate(request->adb_path().c_str(), request->adb_serial().c_str(),
                                         request->adb_type(), request->adb_config().c_str(), CallbackImpl, cbState);

    MAA_GRPC_SET_HANDLE

    response->set_handle(id);

    return Status::OK;
}

Status ControllerImpl::destroy(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                               ::maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE_BEGIN
    handles.erase(id);
    MAA_GRPC_GET_HANDLE_END

    MaaControllerDestroy(handle);

    return Status::OK;
}

Status ControllerImpl::create_custom(::grpc::ServerContext* context, const ::maarpc::CustomControllerRequest* request,
                                     ::grpc::ServerWriter<::maarpc::CustomControllerResponse>* writer)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = writer;

    return Status(UNIMPLEMENTED, "");
}

Status ControllerImpl::submit_custom_controller(::grpc::ServerContext* context,
                                                const ::maarpc::SubmitCustomControllerRequest* request,
                                                ::maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return Status(UNIMPLEMENTED, "");
}

Status ControllerImpl::set_option(::grpc::ServerContext* context, const ::maarpc::ControllerSetOptionRequest* request,
                                  ::maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED_OPTION(option, ControllerSetOptionRequest)

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
            if (MaaControllerSetOption(handle, MaaCtrlOption_DefaultAppPackageEntry, const_cast<char*>(entry.c_str()),
                                       entry.size())) {
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
            if (MaaControllerSetOption(handle, MaaCtrlOption_DefaultAppPackage, const_cast<char*>(entry.c_str()),
                                       entry.size())) {
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
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    response->set_id(MaaControllerPostConnection(handle));

    return Status::OK;
}

Status ControllerImpl::post_click(::grpc::ServerContext* context, const ::maarpc::ControllerPostClickRequest* request,
                                  ::maarpc::IIdResponse* response)
{
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
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    response->set_id(MaaControllerPostConnection(handle));

    return Status::OK;
}

Status ControllerImpl::status(::grpc::ServerContext* context, const ::maarpc::HandleIIdRequest* request,
                              ::maarpc::StatusResponse* response)
{
    std::ignore = context;

    MAA_GRPC_REQUIRED(id)
    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    response->set_status(MaaControllerStatus(handle, request->id()));

    return Status::OK;
}

Status ControllerImpl::wait(::grpc::ServerContext* context, const ::maarpc::HandleIIdRequest* request,
                            ::maarpc::StatusResponse* response)
{
    std::ignore = context;

    MAA_GRPC_REQUIRED(id)
    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    response->set_status(MaaControllerWait(handle, request->id()));

    return Status::OK;
}

Status ControllerImpl::connected(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                                 ::maarpc::BoolResponse* response)
{
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    response->set_bool_(MaaControllerConnected(handle));

    return Status::OK;
}

Status ControllerImpl::image(::grpc::ServerContext* context, const ::maarpc::ControllerGetImageRequest* request,
                             ::maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    return Status(UNIMPLEMENTED, "");
}

Status ControllerImpl::uuid(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                            ::maarpc::StringResponse* response)
{
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
