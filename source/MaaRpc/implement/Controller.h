#pragma once

#include "AtomicMap.h"
#include "Buffer.h"
#include "MaaFramework/Instance/MaaController.h"
#include "Utility.h"
#include "controller.grpc.pb.h"

#include <semaphore>

struct ControllerImpl final : public ::maarpc::Controller::Service
{
    struct CustomControllerInfo
    {
        ::grpc::ServerReaderWriter<::maarpc::CustomControllerResponse, ::maarpc::CustomControllerRequest>* stream;
        ImageImpl* iImpl;
        std::binary_semaphore finish { 0 };
    };

    ControllerImpl(UtilityImpl* uimpl, ImageImpl* iimpl) : uImpl(uimpl), iImpl(iimpl) {}

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

    UtilityImpl* uImpl;
    ImageImpl* iImpl;
    AtomicMap<MaaControllerHandle> handles;
    AtomicMap<CustomControllerInfo*> infos;
};
