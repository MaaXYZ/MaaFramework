#include "SyncContext.h"
#include "MaaFramework/MaaAPI.h"
#include "Macro.h"
#include "Utils/Logger.h"

using namespace ::grpc;

Status SyncContextImpl::run_task(ServerContext* context, const ::maarpc::SyncContextRunTaskRequest* request,
                                 ::maarpc::EmptyResponse* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(task)
    MAA_GRPC_REQUIRED(param)

    MAA_GRPC_GET_HANDLE

    if (MaaSyncContextRunTask(handle, request->task().c_str(), request->param().c_str())) {
        return Status::OK;
    }
    else {
        return Status(UNKNOWN, "MaaSyncContextRunTask failed");
    }
}

Status SyncContextImpl::run_recognizer(ServerContext* context, const ::maarpc::SyncContextRunRecognizerRequest* request,
                                       ::maarpc::SyncContextRunRecognizerResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(task)
    MAA_GRPC_REQUIRED(param)
    MAA_GRPC_REQUIRED(image_handle)

    MAA_GRPC_GET_HANDLE
    MAA_GRPC_GET_HANDLE_FROM(image_impl_, image_handle, image_handle)

    MaaRect box;
    auto dt_handle = MaaCreateStringBuffer();

    if (MaaSyncContextRunRecognizer(handle, image_handle, request->task().c_str(), request->param().c_str(), &box,
                                    dt_handle)) {
        response->mutable_box()->mutable_xy()->set_x(box.x);
        response->mutable_box()->mutable_xy()->set_y(box.y);
        response->mutable_box()->mutable_wh()->set_width(box.width);
        response->mutable_box()->mutable_wh()->set_height(box.height);

        std::string detail(MaaGetString(dt_handle), MaaGetStringSize(dt_handle));
        MaaDestroyStringBuffer(dt_handle);
        response->set_detail(detail);

        return Status::OK;
    }
    else {
        MaaDestroyStringBuffer(dt_handle);
        return Status(UNKNOWN, "MaaSyncContextRunRecognizer failed");
    }
}

Status SyncContextImpl::run_action(ServerContext* context, const ::maarpc::SyncContextRunActionRequest* request,
                                   ::maarpc::EmptyResponse* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(task)
    MAA_GRPC_REQUIRED(param)
    MAA_GRPC_REQUIRED(box)
    MAA_GRPC_REQUIRED(detail)

    MAA_GRPC_GET_HANDLE

    MaaRect box { request->box().xy().x(), request->box().xy().y(), request->box().wh().width(),
                  request->box().wh().height() };
    if (MaaSyncContextRunAction(handle, request->task().c_str(), request->param().c_str(), &box,
                                request->detail().c_str())) {
        return Status::OK;
    }
    else {
        return Status(UNKNOWN, "MaaSyncContextRunAction failed");
    }
}

Status SyncContextImpl::click(ServerContext* context, const ::maarpc::SyncContextClickRequest* request,
                              ::maarpc::EmptyRequest* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(param)

    const auto& param = request->param();

    MAA_GRPC_GET_HANDLE

    if (MaaSyncContextClick(handle, param.point().x(), param.point().y())) {
        return Status::OK;
    }
    else {
        return Status(UNKNOWN, "MaaSyncContextClick failed");
    }
}

Status SyncContextImpl::swipe(ServerContext* context, const ::maarpc::SyncContextSwipeRequest* request,
                              ::maarpc::EmptyRequest* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(param)

    const auto& param = request->param();

    MAA_GRPC_GET_HANDLE

    if (MaaSyncContextSwipe(handle, param.from().x(), param.from().y(), param.to().x(), param.to().y(),
                            param.duration())) {
        return Status::OK;
    }
    else {
        return Status(UNKNOWN, "MaaSyncContextSwipe failed");
    }
}

Status SyncContextImpl::key(ServerContext* context, const ::maarpc::SyncContextKeyRequest* request,
                            ::maarpc::EmptyRequest* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(param)

    const auto& param = request->param();

    MAA_GRPC_GET_HANDLE

    if (MaaSyncContextPressKey(handle, param.key())) {
        return Status::OK;
    }
    else {
        return Status(UNKNOWN, "MaaSyncContextKey failed");
    }
}

Status SyncContextImpl::touch_down(ServerContext* context, const ::maarpc::SyncContextTouchRequest* request,
                                   ::maarpc::EmptyRequest* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(param)

    const auto& param = request->param();

    MAA_GRPC_GET_HANDLE

    if (MaaSyncContextTouchDown(handle, param.contact(), param.pos().x(), param.pos().y(), param.pressure())) {
        return Status::OK;
    }
    else {
        return Status(UNKNOWN, "MaaSyncContextTouchDown failed");
    }
}

Status SyncContextImpl::touch_move(ServerContext* context, const ::maarpc::SyncContextTouchRequest* request,
                                   ::maarpc::EmptyRequest* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(param)

    const auto& param = request->param();

    MAA_GRPC_GET_HANDLE

    if (MaaSyncContextTouchMove(handle, param.contact(), param.pos().x(), param.pos().y(), param.pressure())) {
        return Status::OK;
    }
    else {
        return Status(UNKNOWN, "MaaSyncContextTouchMove failed");
    }
}

Status SyncContextImpl::touch_up(ServerContext* context, const ::maarpc::SyncContextTouchRequest* request,
                                 ::maarpc::EmptyRequest* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(param)

    const auto& param = request->param();

    MAA_GRPC_GET_HANDLE

    if (MaaSyncContextTouchUp(handle, param.contact())) {
        return Status::OK;
    }
    else {
        return Status(UNKNOWN, "MaaSyncContextTouchUp failed");
    }
}

Status SyncContextImpl::screencap(ServerContext* context, const ::maarpc::SyncContextScreencapRequest* request,
                                  ::maarpc::EmptyResponse* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(image_handle)

    MAA_GRPC_GET_HANDLE
    MAA_GRPC_GET_HANDLE_FROM(image_impl_, image_handle, image_handle)

    if (MaaSyncContextScreencap(handle, image_handle)) {
        return Status::OK;
    }
    else {
        return Status(UNKNOWN, "MaaSyncContextScreencap failed");
    }
}

Status SyncContextImpl::task_result(ServerContext* context, const ::maarpc::HandleStringRequest* request,
                                    ::maarpc::StringResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(str)

    MAA_GRPC_GET_HANDLE

    auto dt_handle = MaaCreateStringBuffer();

    if (MaaSyncContextGetTaskResult(handle, request->str().c_str(), dt_handle)) {
        std::string detail(MaaGetString(dt_handle), MaaGetStringSize(dt_handle));
        MaaDestroyStringBuffer(dt_handle);
        response->set_str(detail);
        return Status::OK;
    }
    else {
        MaaDestroyStringBuffer(dt_handle);
        return Status(UNKNOWN, "MaaSyncContextGetTaskResult failed");
    }
}
