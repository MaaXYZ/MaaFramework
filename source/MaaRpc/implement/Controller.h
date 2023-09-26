#pragma once

#include "Utility.h"
#include "controller.grpc.pb.h"

#include <map>

class ControllerImpl final : public ::maarpc::Controller::Service
{
public:
    ControllerImpl(UtilityImpl* impl) : uImpl(impl) {}

    ::grpc::Status create_adb(::grpc::ServerContext* context, const ::maarpc::AdbControllerRequest* request,
                              ::maarpc::HandleResponse* response) override;
    ::grpc::Status destroy(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                           ::maarpc::EmptyResponse* response) override;
    ::grpc::Status create_custom(::grpc::ServerContext* context, const ::maarpc::CustomControllerRequest* request,
                                 ::grpc::ServerWriter<::maarpc::CustomControllerResponse>* writer) override;
    ::grpc::Status submit_custom_controller(::grpc::ServerContext* context,
                                            const ::maarpc::SubmitCustomControllerRequest* request,
                                            ::maarpc::EmptyResponse* response) override;
    ::grpc::Status set_option(::grpc::ServerContext* context, const ::maarpc::ControllerSetOptionRequest* request,
                              ::maarpc::EmptyResponse* response) override;
    ::grpc::Status post_connection(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                                   ::maarpc::IdResponse* response) override;
    ::grpc::Status post_click(::grpc::ServerContext* context, const ::maarpc::ControllerPostClickRequest* request,
                              ::maarpc::IdResponse* response) override;
    ::grpc::Status post_swipe(::grpc::ServerContext* context, const ::maarpc::ControllerPostSwipeRequest* request,
                              ::maarpc::IdResponse* response) override;
    ::grpc::Status post_press_key(::grpc::ServerContext* context, const ::maarpc::ControllerPostKeyRequest* request,
                                  ::maarpc::IdResponse* response) override;
    ::grpc::Status post_touch_down(::grpc::ServerContext* context, const ::maarpc::ControllerPostTouchRequest* request,
                                   ::maarpc::IdResponse* response) override;
    ::grpc::Status post_touch_move(::grpc::ServerContext* context, const ::maarpc::ControllerPostTouchRequest* request,
                                   ::maarpc::IdResponse* response) override;
    ::grpc::Status post_touch_up(::grpc::ServerContext* context, const ::maarpc::ControllerPostTouchRequest* request,
                                 ::maarpc::IdResponse* response) override;
    ::grpc::Status post_screencap(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                                  ::maarpc::IdResponse* response) override;
    ::grpc::Status status(::grpc::ServerContext* context, const ::maarpc::HandleIdRequest* request,
                          ::maarpc::StatusResponse* response) override;
    ::grpc::Status wait(::grpc::ServerContext* context, const ::maarpc::HandleIdRequest* request,
                        ::maarpc::EmptyResponse* response) override;
    ::grpc::Status connected(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                             ::maarpc::BoolResponse* response) override;
    ::grpc::Status image(::grpc::ServerContext* context, const ::maarpc::ControllerGetImageRequest* request,
                         ::maarpc::EmptyResponse* response) override;
    ::grpc::Status uuid(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                        ::maarpc::StringResponse* response) override;

private:
    UtilityImpl* uImpl;
    std::atomic<uint64_t> ctrl_id_counter { 0 };
    std::map<uint64_t, MaaControllerHandle> handles;
    std::mutex handles_mtx;
};
