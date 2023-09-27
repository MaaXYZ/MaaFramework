#include "Device.h"
#include "macro.h"

using namespace ::grpc;

Status DeviceImpl::find(ServerContext* context, const ::maarpc::EmptyRequest* request,
                        ::maarpc::DeviceInfosResponse* response)
{
    std::ignore = context;
    std::ignore = request;

    std::unique_lock<std::mutex> lock(mtx);

    auto size = MaaToolKitFindDevice();

    auto infos = response->mutable_info();
    infos->Clear();
    infos->Reserve(size);

    for (uint64_t i = 0; i < size; i++) {
        auto info = infos->Add();
        info->set_name(MaaToolKitGetDeviceName(i));
        info->set_adb_path(MaaToolKitGetDeviceAdbPath(i));
        info->set_adb_serial(MaaToolKitGetDeviceAdbSerial(i));
        info->set_adb_type(MaaToolKitGetDeviceAdbControllerType(i));
        info->set_adb_config(MaaToolKitGetDeviceAdbConfig(i));
    }

    return Status::OK;
}

Status DeviceImpl::find_with_adb(ServerContext* context, const ::maarpc::StringRequest* request,
                                 ::maarpc::DeviceInfosResponse* response)
{
    std::ignore = context;

    auto adb = request->str();

    std::unique_lock<std::mutex> lock(mtx);

    auto size = MaaToolKitFindDeviceWithAdb(adb.c_str());

    auto infos = response->mutable_info();
    infos->Clear();
    infos->Reserve(size);

    for (uint64_t i = 0; i < size; i++) {
        auto info = infos->Add();
        info->set_name(MaaToolKitGetDeviceName(i));
        info->set_adb_path(MaaToolKitGetDeviceAdbPath(i));
        info->set_adb_serial(MaaToolKitGetDeviceAdbSerial(i));
        info->set_adb_type(MaaToolKitGetDeviceAdbControllerType(i));
        info->set_adb_config(MaaToolKitGetDeviceAdbConfig(i));
    }

    return Status::OK;
}
