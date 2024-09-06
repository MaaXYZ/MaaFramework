#pragma once

#include <filesystem>
#include <memory>
#include <shared_mutex>

#define BOOST_DLL_USE_STD_FS 1
#include <boost/dll.hpp>
#include <boost/function.hpp>

#include "Conf/Conf.h"
#include "MaaFramework/Plugin/PluginHook.h"
#include "Utils/SingletonHolder.hpp"

MAA_NS_BEGIN

using MaaPluginHook = MaaPluginHook_V0;

class PluginHolder
{
public:
    PluginHolder() { reset_hook(); }

    ~PluginHolder() { unload(); }

    PluginHolder(const PluginHolder&) = delete;
    PluginHolder& operator=(const PluginHolder&) = delete;

    bool load(std::filesystem::path path);
    void unload();
    void reset_hook();

    const MaaPluginHook& hook() const { return hook_; }

private:
    boost::dll::shared_library module_;
    MaaPluginHook hook_;
};

class PluginManager : public SingletonHolder<PluginManager>
{
public:
    PluginManager() { scan_load(); }

    void call_on_instance_created(MaaInstanceHandle inst);
    void call_on_instance_destroyed(MaaInstanceHandle inst);

    void scan_load();

private:
    std::vector<std::unique_ptr<PluginHolder>> plugins_;
    std::shared_mutex mutex_;
};

MAA_NS_END
