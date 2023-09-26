#pragma once

#include "AtomicMap.h"
#include "Buffer.h"
#include "MaaFramework/Task/MaaSyncContext.h"
#include "sync.context.grpc.pb.h"

struct SyncContextImpl final : public ::maarpc::SyncContext::Service
{
    SyncContextImpl(ImageImpl* iimpl) : iImpl(iimpl) {}

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

    ImageImpl* iImpl;
    AtomicMap<MaaSyncContextHandle> handles;
};
