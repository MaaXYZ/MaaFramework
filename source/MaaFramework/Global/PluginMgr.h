#pragma once

#define BOOST_DLL_USE_STD_FS 1

#include <condition_variable>
#include <filesystem>
#include <mutex>
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
        void (*on_init)() = nullptr;
        void (*on_flush)() = nullptr;
        void (*on_shutdown)() = nullptr;
        bool initialized = false;
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
    void instance_created();
    void instance_destroyed();
    bool flush();
    bool shutdown();
    std::vector<Sink> get_res_sinks() const;
    std::vector<Sink> get_ctrl_sinks() const;
    std::vector<Sink> get_tasker_sinks() const;
    std::vector<Sink> get_ctx_sinks() const;

private:
    std::optional<boost::dll::shared_library> load_dll(const std::filesystem::path& library_path);
    bool load_and_parse(const std::filesystem::path& library_path);

private:
    void initialize(std::unique_lock<std::mutex>& lock);

    mutable std::mutex mutex_;
    std::condition_variable management_done_;
    size_t instances_ = 0;
    bool shutdown_ = false;
    bool managing_ = false;
    std::unordered_map<std::string, Plugin> plugins_;
};

MAA_GLOBAL_NS_END
