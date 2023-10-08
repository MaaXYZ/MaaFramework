#pragma once

#include "MaaToolKit/Device/MaaToolKitDevice.h"
#include "device.grpc.pb.h"

struct DeviceImpl final : public ::maarpc::Device::Service
{
    ::grpc::Status find(::grpc::ServerContext* context, const ::maarpc::EmptyRequest* request,
                        ::maarpc::DeviceInfosResponse* response) override;
    ::grpc::Status find_with_adb(::grpc::ServerContext* context, const ::maarpc::StringRequest* request,
                                 ::maarpc::DeviceInfosResponse* response) override;

    std::mutex mtx;
};
