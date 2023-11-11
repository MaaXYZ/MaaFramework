#pragma once

#include "AtomicMap.h"
#include "Buffer.h"
#include "MaaFramework/MaaDef.h"
#include "Utility.h"
#include "generated/controller.grpc.pb.h"

#include <semaphore>

class ControllerImpl final : public ::maarpc::Controller::Service
{
public:
    struct CustomControllerInfo
    {
        ::grpc::ServerReaderWriter<::maarpc::CustomControllerResponse, ::maarpc::CustomControllerRequest>* stream =
            nullptr;
        std::shared_ptr<ImageImpl> image_impl = nullptr;
        std::binary_semaphore finish { 0 };
    };

public:
    ControllerImpl(std::shared_ptr<UtilityImpl> uimpl, std::shared_ptr<ImageImpl> iimpl)
        : utility_impl_(std::move(uimpl)), image_impl_(std::move(iimpl))
    {}

    ::grpc::Status create_adb(::grpc::ServerContext* context, const ::maarpc::AdbControllerRequest* request,
                              ::maarpc::HandleResponse* response) override;
    ::grpc::Status destroy(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                           ::maarpc::EmptyResponse* response) override;
    ::grpc::Status create_custom(::grpc::ServerContext* context,
                                 ::grpc::ServerReaderWriter<::maarpc::CustomControllerResponse,
                                                            ::maarpc::CustomControllerRequest>* stream) override;
    ::grpc::Status set_option(::grpc::ServerContext* context, const ::maarpc::ControllerSetOptionRequest* request,
                              ::maarpc::EmptyResponse* response) override;
    ::grpc::Status post_connection(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                                   ::maarpc::IIdResponse* response) override;
    ::grpc::Status post_click(::grpc::ServerContext* context, const ::maarpc::ControllerPostClickRequest* request,
                              ::maarpc::IIdResponse* response) override;
    ::grpc::Status post_swipe(::grpc::ServerContext* context, const ::maarpc::ControllerPostSwipeRequest* request,
                              ::maarpc::IIdResponse* response) override;
    ::grpc::Status post_press_key(::grpc::ServerContext* context, const ::maarpc::ControllerPostKeyRequest* request,
                                  ::maarpc::IIdResponse* response) override;
    ::grpc::Status post_touch_down(::grpc::ServerContext* context, const ::maarpc::ControllerPostTouchRequest* request,
                                   ::maarpc::IIdResponse* response) override;
    ::grpc::Status post_touch_move(::grpc::ServerContext* context, const ::maarpc::ControllerPostTouchRequest* request,
                                   ::maarpc::IIdResponse* response) override;
    ::grpc::Status post_touch_up(::grpc::ServerContext* context, const ::maarpc::ControllerPostTouchRequest* request,
                                 ::maarpc::IIdResponse* response) override;
    ::grpc::Status post_screencap(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                                  ::maarpc::IIdResponse* response) override;
    ::grpc::Status status(::grpc::ServerContext* context, const ::maarpc::HandleIIdRequest* request,
                          ::maarpc::StatusResponse* response) override;
    ::grpc::Status wait(::grpc::ServerContext* context, const ::maarpc::HandleIIdRequest* request,
                        ::maarpc::StatusResponse* response) override;
    ::grpc::Status connected(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                             ::maarpc::BoolResponse* response) override;
    ::grpc::Status image(::grpc::ServerContext* context, const ::maarpc::HandleHandleRequest* request,
                         ::maarpc::EmptyResponse* response) override;
    ::grpc::Status uuid(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                        ::maarpc::StringResponse* response) override;

    AtomicMap<MaaControllerHandle>& handles() { return handles_; }

private:
    std::shared_ptr<UtilityImpl> utility_impl_ = nullptr;
    std::shared_ptr<ImageImpl> image_impl_ = nullptr;
    AtomicMap<MaaControllerHandle> handles_;
    AtomicMap<std::shared_ptr<CustomControllerInfo>> infos_;
};
