#include "Device.h"
#include "MaaToolkit/MaaToolkitAPI.h"
#include "Macro.h"
#include "Utils/Logger.h"

MAA_RPC_NS_BEGIN

using namespace ::grpc;

Status DeviceImpl::find(ServerContext* context, const ::maarpc::EmptyRequest* request,
                        ::maarpc::DeviceInfosResponse* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = request;

    std::unique_lock<std::mutex> lock(mtx_);

    auto size = MaaToolkitFindDevice();

    auto infos_ = response->mutable_info();
    infos_->Clear();
    // infos->Reserve(size);

    for (uint64_t i = 0; i < size; i++) {
        auto info = infos_->Add();
        info->set_name(MaaToolkitGetDeviceName(i));
        info->set_adb_path(MaaToolkitGetDeviceAdbPath(i));
        info->set_adb_serial(MaaToolkitGetDeviceAdbSerial(i));
        info->set_adb_type(MaaToolkitGetDeviceAdbControllerType(i));
        info->set_adb_config(MaaToolkitGetDeviceAdbConfig(i));
    }

    return Status::OK;
}

Status DeviceImpl::find_with_adb(ServerContext* context, const ::maarpc::StringRequest* request,
                                 ::maarpc::DeviceInfosResponse* response)
{
    LogFunc;
    std::ignore = context;

    auto adb = request->str();

    std::unique_lock<std::mutex> lock(mtx_);

    auto size = MaaToolkitFindDeviceWithAdb(adb.c_str());

    auto infos_ = response->mutable_info();
    infos_->Clear();
    // infos->Reserve(size);

    for (uint64_t i = 0; i < size; i++) {
        auto info = infos_->Add();
        info->set_name(MaaToolkitGetDeviceName(i));
        info->set_adb_path(MaaToolkitGetDeviceAdbPath(i));
        info->set_adb_serial(MaaToolkitGetDeviceAdbSerial(i));
        info->set_adb_type(MaaToolkitGetDeviceAdbControllerType(i));
        info->set_adb_config(MaaToolkitGetDeviceAdbConfig(i));
    }

    return Status::OK;
}

MAA_RPC_NS_END
