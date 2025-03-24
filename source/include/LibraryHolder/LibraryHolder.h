#pragma once

#include <filesystem>
#include <mutex>
#include <type_traits>

#define BOOST_DLL_USE_STD_FS 1
#include <boost/dll.hpp>
#include <boost/function.hpp>

#include "Conf/Conf.h"
#include "Utils/Logger.h"

MAA_NS_BEGIN

// template for ref_count
template <typename T>
class LibraryHolder
{
public:
    virtual ~LibraryHolder();

    static bool load_library(const std::filesystem::path& libname);
    static void unload_library();

    template <typename FuncT>
    static boost::function<FuncT> get_function(const std::string& func_name);

protected:
    LibraryHolder() = default;

private:
    inline static std::filesystem::path libname_;
    inline static std::mutex mutex_;

    inline static boost::dll::shared_library module_;
};

template <typename T>
inline LibraryHolder<T>::~LibraryHolder()
{
    unload_library();
}

template <typename T>
inline bool LibraryHolder<T>::load_library(const std::filesystem::path& libname)
{
    LogFunc << VAR(libname);

    std::unique_lock lock(mutex_);

    if (module_.is_loaded()) {
        if (libname_ != libname) {
            LogError << "Already loaded with different library" << VAR(libname_) << VAR(libname);
            return false;
        }

        LogDebug << "Already loaded";
        return true;
    }

    LogInfo << "Loading library" << VAR(libname);

    boost::dll::fs::error_code ec;
    module_.load(libname, ec, boost::dll::load_mode::append_decorations | boost::dll::load_mode::search_system_folders);

    if (ec) {
        auto message = ec.message();
        LogError << "Failed to load library" << VAR(libname) << VAR(message);
        return false;
    }

    if (!module_.is_loaded()) {
        LogError << "Failed to load library" << VAR(libname);
        return false;
    }

    libname_ = libname;
    return true;
}

template <typename T>
inline void LibraryHolder<T>::unload_library()
{
    LogFunc << VAR(libname_);

    std::unique_lock lock(mutex_);

    if (!module_.is_loaded()) {
        LogDebug << "LibraryHolder already unloaded";
        return;
    }

    LogInfo << "Unloading library" << VAR(libname_);

    module_.unload();
}

template <typename T>
template <typename FuncT>
inline boost::function<FuncT> LibraryHolder<T>::get_function(const std::string& func_name)
{
    LogFunc << VAR(func_name);

    std::unique_lock lock(mutex_);

    if (!module_.is_loaded()) {
        LogError << "LibraryHolder not loaded";
        return {};
    }

    if (!module_.has(func_name)) {
        LogError << "Failed to find exported function" << VAR(func_name);
        return {};
    }

    return module_.get<FuncT>(func_name);
}

MAA_NS_END
