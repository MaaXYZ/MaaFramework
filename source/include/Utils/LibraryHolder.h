#pragma once

#include <filesystem>
#include <mutex>
#include <type_traits>

#include "Conf/Conf.h"
#include "NonCopyable.hpp"
#include "Utils/Boost.hpp"
#include "Utils/Logger.h"

MAA_NS_BEGIN

// template for ref_count
template <typename T>
class LibraryHolder
{
public:
    virtual ~LibraryHolder();

    static bool load_library(const std::filesystem::path& libname);

    template <typename FuncT>
    static boost::function<FuncT> get_function(const std::string& func_name);

protected:
    LibraryHolder() = default;

private:
    static void unload_library();

private:
    inline static std::filesystem::path libname_;
    inline static int ref_count_ = 0;
    inline static std::mutex mutex_;

    inline static boost::dll::shared_library module_;
};

template <typename T>
inline LibraryHolder<T>::~LibraryHolder()
{
    LogFunc;

    unload_library();
}

template <typename T>
inline bool LibraryHolder<T>::load_library(const std::filesystem::path& libname)
{
    LogFunc << VAR(libname);

    std::unique_lock<std::mutex> lock(mutex_);

    if (module_.is_loaded()) {
        if (libname_ != libname) {
            LogError << "already loaded with different library" << VAR(libname_) << VAR(libname);
            return false;
        }

        ++ref_count_;
        LogDebug << "already loaded" << VAR(ref_count_);
        return true;
    }

    LogInfo << "Loading library" << VAR(libname);

    boost::dll::fs::error_code ec;
    module_.load(libname, ec, boost::dll::load_mode::append_decorations | boost::dll::load_mode::search_system_folders);

    if (ec.value() != boost::system::errc::success) {
        auto message = ec.message();
        LogError << "Failed to load library" << VAR(libname) << VAR(message);
        return false;
    }

    if (!module_.is_loaded()) {
        LogError << "Failed to load library" << VAR(libname);
        return false;
    }

    libname_ = libname;
    ++ref_count_;
    return true;
}

template <typename T>
inline void LibraryHolder<T>::unload_library()
{
    LogFunc << VAR(libname_);

    std::unique_lock<std::mutex> lock(mutex_);

    if (!module_.is_loaded()) {
        LogDebug << "LibraryHolder already unloaded";
        return;
    }

    --ref_count_;
    if (ref_count_ > 0) {
        LogDebug << "LibraryHolder ref count" << VAR(ref_count_);
        return;
    }

    LogInfo << "Unloading library" << VAR(libname_);

    module_.unload();

    libname_.clear();
    ref_count_ = 0;
}

template <typename T>
template <typename FuncT>
inline boost::function<FuncT> LibraryHolder<T>::get_function(const std::string& func_name)
{
    LogFunc << VAR(func_name);

    std::unique_lock<std::mutex> lock(mutex_);

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
