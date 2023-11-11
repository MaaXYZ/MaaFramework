#pragma once

#include "AtomicMap.h"
#include "Buffer.h"
#include "MaaFramework/MaaDef.h"
#include "generated/sync.context.grpc.pb.h"

class SyncContextImpl final : public ::maarpc::SyncContext::Service
{
public:
    SyncContextImpl(std::shared_ptr<ImageImpl> iimpl) : image_impl_(std::move(iimpl)) {}

    ::grpc::Status run_task(::grpc::ServerContext* context, const ::maarpc::SyncContextRunTaskRequest* request,
                            ::maarpc::EmptyResponse* response) override;
    ::grpc::Status run_recognizer(::grpc::ServerContext* context,
                                  const ::maarpc::SyncContextRunRecognizerRequest* request,
                                  ::maarpc::SyncContextRunRecognizerResponse* response) override;
    ::grpc::Status run_action(::grpc::ServerContext* context, const ::maarpc::SyncContextRunActionRequest* request,
                              ::maarpc::EmptyResponse* response) override;
    ::grpc::Status click(::grpc::ServerContext* context, const ::maarpc::SyncContextClickRequest* request,
                         ::maarpc::EmptyRequest* response) override;
    ::grpc::Status swipe(::grpc::ServerContext* context, const ::maarpc::SyncContextSwipeRequest* request,
                         ::maarpc::EmptyRequest* response) override;
    ::grpc::Status key(::grpc::ServerContext* context, const ::maarpc::SyncContextKeyRequest* request,
                       ::maarpc::EmptyRequest* response) override;
    ::grpc::Status touch_down(::grpc::ServerContext* context, const ::maarpc::SyncContextTouchRequest* request,
                              ::maarpc::EmptyRequest* response) override;
    ::grpc::Status touch_move(::grpc::ServerContext* context, const ::maarpc::SyncContextTouchRequest* request,
                              ::maarpc::EmptyRequest* response) override;
    ::grpc::Status touch_up(::grpc::ServerContext* context, const ::maarpc::SyncContextTouchRequest* request,
                            ::maarpc::EmptyRequest* response) override;
    ::grpc::Status screencap(::grpc::ServerContext* context, const ::maarpc::SyncContextScreencapRequest* request,
                             ::maarpc::EmptyResponse* response) override;
    ::grpc::Status task_result(::grpc::ServerContext* context, const ::maarpc::HandleStringRequest* request,
                               ::maarpc::StringResponse* response) override;

    AtomicMap<MaaSyncContextHandle>& handles() { return handles_; }

private:
    std::shared_ptr<ImageImpl> image_impl_ = nullptr;
    AtomicMap<MaaSyncContextHandle> handles_;
};
