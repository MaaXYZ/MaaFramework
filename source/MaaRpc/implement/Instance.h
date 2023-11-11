#pragma once

#include "AtomicMap.h"
#include "Buffer.h"
#include "Controller.h"
#include "MaaFramework/MaaDef.h"
#include "Resource.h"
#include "SyncContext.h"
#include "Utility.h"
#include "generated/instance.grpc.pb.h"

class InstanceImpl final : public ::maarpc::Instance::Service
{
public:
    struct CustomRecognizerInfo
    {
        std::string name;
        ::grpc::ServerReaderWriter<::maarpc::CustomRecognizerResponse, ::maarpc::CustomRecognizerRequest>* stream =
            nullptr;
        std::shared_ptr<ImageImpl> image_impl = nullptr;
        std::shared_ptr<SyncContextImpl> syncctx_impl = nullptr;
        std::binary_semaphore finish { 0 };
    };
    struct CustomActionInfo
    {
        std::string name;
        ::grpc::ServerReaderWriter<::maarpc::CustomActionResponse, ::maarpc::CustomActionRequest>* stream = nullptr;
        std::shared_ptr<SyncContextImpl> syncctx_impl = nullptr;
        std::binary_semaphore finish { 0 };
    };

public:
    InstanceImpl(std::shared_ptr<UtilityImpl> uimpl, std::shared_ptr<ImageImpl> iimpl,
                 std::shared_ptr<ResourceImpl> rimpl, std::shared_ptr<ControllerImpl> cimpl,
                 std::shared_ptr<SyncContextImpl> simpl)
        : utility_impl_(std::move(uimpl)), image_impl_(std::move(iimpl)), resource_impl_(std::move(rimpl)),
          controller_impl_(std::move(cimpl)), syncctx_impl_(std::move(simpl))
    {}

    ::grpc::Status create(::grpc::ServerContext* context, const ::maarpc::IdRequest* request,
                          ::maarpc::HandleResponse* response) override;
    ::grpc::Status destroy(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                           ::maarpc::EmptyResponse* response) override;
    ::grpc::Status register_custom_recognizer(
        ::grpc::ServerContext* context,
        ::grpc::ServerReaderWriter<::maarpc::CustomRecognizerResponse, ::maarpc::CustomRecognizerRequest>* stream)
        override;
    ::grpc::Status unregister_custom_recognizer(::grpc::ServerContext* context,
                                                const ::maarpc::HandleStringRequest* request,
                                                ::maarpc::EmptyResponse* response) override;
    ::grpc::Status clear_custom_recognizer(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                                           ::maarpc::EmptyResponse* response) override;
    ::grpc::Status register_custom_action(
        ::grpc::ServerContext* context,
        ::grpc::ServerReaderWriter<::maarpc::CustomActionResponse, ::maarpc::CustomActionRequest>* stream) override;
    ::grpc::Status unregister_custom_action(::grpc::ServerContext* context,
                                            const ::maarpc::HandleStringRequest* request,
                                            ::maarpc::EmptyResponse* response) override;
    ::grpc::Status clear_custom_action(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                                       ::maarpc::EmptyResponse* response) override;
    ::grpc::Status bind_resource(::grpc::ServerContext* context, const ::maarpc::HandleHandleRequest* request,
                                 ::maarpc::EmptyResponse* response) override;
    ::grpc::Status bind_controller(::grpc::ServerContext* context, const ::maarpc::HandleHandleRequest* request,
                                   ::maarpc::EmptyResponse* response) override;
    ::grpc::Status inited(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                          ::maarpc::BoolResponse* response) override;
    ::grpc::Status post_task(::grpc::ServerContext* context, const ::maarpc::InstancePostTaskRequest* request,
                             ::maarpc::IIdResponse* response) override;
    ::grpc::Status set_task_param(::grpc::ServerContext* context, const ::maarpc::InstanceSetTaskParamRequest* request,
                                  ::maarpc::EmptyResponse* response) override;
    ::grpc::Status status(::grpc::ServerContext* context, const ::maarpc::HandleIIdRequest* request,
                          ::maarpc::StatusResponse* response) override;
    ::grpc::Status wait(::grpc::ServerContext* context, const ::maarpc::HandleIIdRequest* request,
                        ::maarpc::StatusResponse* response) override;
    ::grpc::Status all_finished(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                                ::maarpc::BoolResponse* response) override;
    ::grpc::Status stop(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                        ::maarpc::EmptyResponse* response) override;
    ::grpc::Status resource(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                            ::maarpc::HandleRequest* response) override;
    ::grpc::Status controller(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                              ::maarpc::HandleRequest* response) override;

    AtomicMap<MaaInstanceHandle>& handles() { return handles_; }

private:
    std::shared_ptr<UtilityImpl> utility_impl_ = nullptr;
    std::shared_ptr<ImageImpl> image_impl_ = nullptr;
    std::shared_ptr<ResourceImpl> resource_impl_ = nullptr;
    std::shared_ptr<ControllerImpl> controller_impl_ = nullptr;
    std::shared_ptr<SyncContextImpl> syncctx_impl_ = nullptr;

    AtomicMap<MaaInstanceHandle> handles_;
    AtomicMap<std::shared_ptr<CustomRecognizerInfo>> recos_;
    AtomicMap<MaaInstanceHandle, std::shared_ptr<CustomRecognizerInfo>> reco_idx_;
    AtomicMap<std::shared_ptr<CustomActionInfo>> actions_;
    AtomicMap<MaaInstanceHandle, std::shared_ptr<CustomActionInfo>> action_idx_;
};
