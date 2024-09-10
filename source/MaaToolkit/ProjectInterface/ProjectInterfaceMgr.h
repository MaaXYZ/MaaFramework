#pragma once

#include <filesystem>
#include <map>
#include <string>

#include "Conf/Conf.h"
#include "MaaFramework/MaaDef.h"
#include "ProjectInterface/Runner.h"
#include "Utils/SingletonHolder.hpp"

MAA_TOOLKIT_NS_BEGIN

class ProjectInterfaceMgr : public SingletonHolder<ProjectInterfaceMgr>
{
    friend class SingletonHolder<ProjectInterfaceMgr>;

public:
    void register_custom_recognizer(const std::string& name, MAA_PROJECT_INTERFACE_NS::CustomRecognizerSession recognizer);
    void register_custom_action(const std::string& name, MAA_PROJECT_INTERFACE_NS::CustomActionSession action);

    bool run_cli(
        const std::filesystem::path& resource_path,
        const std::filesystem::path& user_path,
        bool directly,
        MaaNotificationCallback callback,
        void* callback_arg);

private:
    std::map<std::string, MAA_PROJECT_INTERFACE_NS::CustomRecognizerSession> custom_recognizers_;
    std::map<std::string, MAA_PROJECT_INTERFACE_NS::CustomActionSession> custom_actions_;
};

MAA_TOOLKIT_NS_END
