#pragma once

#define BOOST_DLL_USE_STD_FS 1

#include <filesystem>
#include <unordered_map>

#include <boost/dll.hpp>
#include <boost/function.hpp>

#include "Common/Conf.h"
#include "MaaFramework/MaaDef.h"
#include "MaaPlugin/MaaPluginAPI.h"
#include "MaaUtils/LibraryHolder.h"
#include "MaaUtils/SingletonHolder.hpp"

MAA_GLOBAL_NS_BEGIN

class PluginMgr : public SingletonHolder<PluginMgr>
{
public:
    using Sink = void (*)(void* handle, const char* message, const char* details_json, void* trans_arg);

    struct Plugin
    {
        boost::dll::shared_library library;
        Sink on_res_event = nullptr;
        Sink on_ctrl_event = nullptr;
        Sink on_tasker_event = nullptr;
        Sink on_ctx_event = nullptr;
    };

public:
    friend class SingletonHolder<PluginMgr>;

public:
    bool load(const std::filesystem::path& library_path);

    std::vector<std::string> get_names() const;
    std::vector<Sink> get_res_sinks() const;
    std::vector<Sink> get_ctrl_sinks() const;
    std::vector<Sink> get_tasker_sinks() const;
    std::vector<Sink> get_ctx_sinks() const;

private:
    std::optional<boost::dll::shared_library> load_dll(const std::filesystem::path& library_path);
    bool load_and_parse(const std::filesystem::path& library_path);

private:
    std::unordered_map<std::string, Plugin> plugins_;
};

MAA_GLOBAL_NS_END
