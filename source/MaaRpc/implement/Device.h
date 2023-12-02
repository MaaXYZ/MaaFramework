#pragma once

#include "Conf/Conf.h"

#include "MaaToolKit/Device/MaaToolKitDevice.h"
#include "generated/device.grpc.pb.h"

MAA_NS_BEGIN

class DeviceImpl final : public ::maarpc::Device::Service
{
public:
    ::grpc::Status find(::grpc::ServerContext* context, const ::maarpc::EmptyRequest* request,
                        ::maarpc::DeviceInfosResponse* response) override;
    ::grpc::Status find_with_adb(::grpc::ServerContext* context, const ::maarpc::StringRequest* request,
                                 ::maarpc::DeviceInfosResponse* response) override;

private:
    std::mutex mtx_;
};

MAA_NS_END
