#pragma once

#include "Conf/Conf.h"
#include "Impl/PIData.h"
#include "PI/Types.h"

#include <variant>

MAA_PI_NS_BEGIN

class PIConfig : public MaaPIConfig
{
    friend class PIRuntime;
    friend class PIClient;

public:
    using AdbConfigInfo = ConfigJson::AdbConfig;

    struct DesktopConfigInfo
    {
        void* hwnd = nullptr;
        std::wstring class_name = L"";
        std::wstring window_name = L"";
    };

    struct TaskConfigInfo
    {
        struct OptionConfigInfo
        {
            PIData::OptionInfo* option_ = nullptr;
            PIData::OptionInfo::CaseInfo* case_ = nullptr;
        };

        PIData::TaskInfo* task_ = nullptr;
        std::vector<OptionConfigInfo> option_ = {};
    };

    virtual bool load(MaaPIData* data, std::string_view json) override;
    virtual bool gen_default(MaaPIData* data) override;
    virtual bool save(std::string& json) override;

private:
    PIData* data_ = nullptr;
    PIData::ControllerInfo* controller_ = nullptr;
    std::variant<std::monostate, AdbConfigInfo, DesktopConfigInfo> controller_info_;
    PIData::ResourceInfo* resource_ = nullptr;
    std::vector<TaskConfigInfo> task_;
};

MAA_PI_NS_END
