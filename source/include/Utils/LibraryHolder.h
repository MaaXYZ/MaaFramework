#pragma once

#include <filesystem>
#include <mutex>
#include <type_traits>

#ifdef _WIN32
#include "SafeWindows.hpp"
#else
#include <dlfcn.h>
#endif

#include "Conf/Conf.h"
#include "NonCopyable.hpp"
#include "Utils/Logger.h"

MAA_NS_BEGIN

// template for ref_count
template <typename T>
class LibraryHolder : public NonCopyable
{
public:
    LibraryHolder() = default;
    virtual ~LibraryHolder();

    static bool load_library(const std::filesystem::path& libname);

    template <typename FuncT>
    static FuncT get_function(const std::string& func_name);

private:
    static void unload_library();

private:
    inline static std::filesystem::path libname_;
    inline static int ref_count_ = 0;

#ifdef _WIN32
    inline static HMODULE module_ = nullptr;
#else
    inline static void* module_ = nullptr;
#endif
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

    if (module_) {
        if (libname_ != libname) {
            LogError << "already loaded with different library" << VAR(libname_) << VAR(libname);
            return false;
        }

        ++ref_count_;
        LogDebug << "already loaded" << VAR(ref_count_);
        return true;
    }

    std::string s_libname = path_to_crt_string(libname);
    LogInfo << "Loading library" << VAR(s_libname);

#ifdef _WIN32
    module_ = LoadLibrary(s_libname.c_str());
#else
    module_ = dlopen(libname_.c_str(), RTLD_LAZY);
#endif

    if (module_ == nullptr) {
        LogError << "Failed to load library" << VAR(s_libname);
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

    if (module_ == nullptr) {
        LogDebug << "LibraryHolder already unloaded";
        return;
    }

    --ref_count_;
    if (ref_count_ > 0) {
        LogDebug << "LibraryHolder ref count" << VAR(ref_count_);
        return;
    }

    LogInfo << "Unloading library" << VAR(libname_);

#ifdef _WIN32
    FreeLibrary(module_);
#else
    dlclose(module_);
#endif

    module_ = nullptr;
    libname_.clear();
    ref_count_ = 0;
}

template <typename T>
template <typename FuncT>
inline FuncT LibraryHolder<T>::get_function(const std::string& func_name)
{
    LogFunc << VAR(func_name);

    if (module_ == nullptr) {
        LogError << "LibraryHolder not loaded";
        return nullptr;
    }

#ifdef _WIN32
    FARPROC proc_address = GetProcAddress(module_, func_name.c_str());
#else
    void* proc_address = dlsym(module_, func_name.c_str());
#endif

    if (proc_address == nullptr) {
        LogError << "Failed to get function address" << VAR(func_name);
        return nullptr;
    }

    return reinterpret_cast<FuncT>(proc_address);
}

MAA_NS_END
