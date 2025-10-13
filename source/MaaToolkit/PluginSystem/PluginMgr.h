#pragma once

#define BOOST_DLL_USE_STD_FS 1

#include <filesystem>
#include <unordered_map>

#include <boost/dll.hpp>
#include <boost/function.hpp>

#include "Conf/Conf.h"
#include "LibraryHolder/LibraryHolder.h"
#include "MaaFramework/MaaDef.h"
#include "Utils/SingletonHolder.hpp"

MAA_TOOLKIT_NS_BEGIN

class PluginMgr : public SingletonHolder<PluginMgr>
{
    friend class SingletonHolder<PluginMgr>;

public:
    bool load_library(const std::filesystem::path& library_name, MaaTasker* tasker, MaaResource* resource, MaaController* controller);
    bool load_directory(const std::filesystem::path& directory_path, MaaTasker* tasker, MaaResource* resource, MaaController* controller);

private:
    std::optional<boost::dll::shared_library> load_dll(const std::filesystem::path& library_path);
    bool load_and_register(const std::filesystem::path& library_path, MaaTasker* tasker, MaaResource* resource, MaaController* controller);

private:
    std::unordered_map<std::string, boost::dll::shared_library> libraries_;
};

MAA_TOOLKIT_NS_END
