#include "AndroidExternalLib.h"
#include <utility>

#include "MaaUtils/Logger.h"

namespace
{
template <typename FuncT>
bool get_required_function(const std::string& func_name, boost::function<FuncT>& output)
{
    output = MAA_NS::AndroidNativeExternalLibraryHolder::get_function<FuncT>(func_name);
    if (output) {
        return true;
    }

    LogError << "Failed to get function" << VAR(func_name);
    return false;
}
} // namespace

MAA_NS_BEGIN

std::optional<AndroidNativeNS::AndroidExternalFunctions>
    AndroidNativeExternalLibraryHolder::create_functions(const std::filesystem::path& library_path)
{
    LogFunc << VAR(library_path);

    if (!load_library(library_path)) {
        LogError << "Failed to load library" << VAR(library_path);
        return std::nullopt;
    }

    AndroidNativeNS::AndroidExternalFunctions funcs;
    bool ok = true;

    ok = ok && get_required_function<AndroidNativeNS::GetLockedPixelsSignature>(get_locked_pixels_func_name_, funcs.get_locked_pixels);
    ok = ok && get_required_function<AndroidNativeNS::UnlockPixelsSignature>(unlock_pixels_func_name_, funcs.unlock_pixels);
    ok = ok && get_required_function<AndroidNativeNS::AttachThreadSignature>(attach_thread_func_name_, funcs.attach_thread);
    ok = ok && get_required_function<AndroidNativeNS::DetachThreadSignature>(detach_thread_func_name_, funcs.detach_thread);
    ok = ok && get_required_function<AndroidNativeNS::DispatchInputMessageSignature>(
        dispatch_input_message_func_name_,
        funcs.dispatch_input_message);

    if (!ok) {
        unload_library();
        return std::nullopt;
    }

    return funcs;
}

MAA_NS_END

namespace AndroidNativeNS
{

AndroidExternalLib::~AndroidExternalLib()
{
    unload();
}

bool AndroidExternalLib::load(const std::filesystem::path& library_path)
{
    LogFunc << VAR(library_path);

    if (library_path.empty()) {
        LogError << "library_path is empty";
        return false;
    }

    if (loaded_) {
        if (library_path_ == library_path) {
            return true;
        }

        LogError << "AndroidExternalLib instance already loaded with different library_path" << VAR(library_path_) << VAR(library_path);
        return false;
    }

    auto funcs_opt = MAA_NS::AndroidNativeExternalLibraryHolder::create_functions(library_path);
    if (!funcs_opt) {
        clear_functions();
        library_path_.clear();
        loaded_ = false;
        return false;
    }

    funcs_ = std::move(*funcs_opt);
    library_path_ = library_path;
    loaded_ = true;
    return true;
}

void AndroidExternalLib::unload()
{
    if (!loaded_) {
        clear_functions();
        library_path_.clear();
        return;
    }

    LogFunc << VAR(library_path_);

    MAA_NS::AndroidNativeExternalLibraryHolder::unload_library();
    clear_functions();
    library_path_.clear();
    loaded_ = false;
}

bool AndroidExternalLib::loaded() const
{
    return loaded_ && funcs_.get_locked_pixels && funcs_.unlock_pixels && funcs_.attach_thread && funcs_.detach_thread &&
           funcs_.dispatch_input_message;
}

const std::filesystem::path& AndroidExternalLib::library_path() const
{
    return library_path_;
}

FrameInfo AndroidExternalLib::GetLockedPixels() const
{
    if (!loaded()) {
        LogError << "android external library is not loaded";
        return {};
    }

    return funcs_.get_locked_pixels();
}

int AndroidExternalLib::UnlockPixels(FrameInfo info) const
{
    if (!loaded()) {
        LogError << "android external library is not loaded";
        return -1;
    }

    return funcs_.unlock_pixels(info);
}

void* AndroidExternalLib::AttachThread() const
{
    if (!loaded()) {
        LogError << "android external library is not loaded";
        return nullptr;
    }

    return funcs_.attach_thread();
}

int AndroidExternalLib::DetachThread(void* env) const
{
    if (!loaded()) {
        LogError << "android external library is not loaded";
        return -1;
    }

    return funcs_.detach_thread(env);
}

int AndroidExternalLib::DispatchInputMessage(MethodParam param) const
{
    if (!loaded()) {
        LogError << "android external library is not loaded";
        return -1;
    }

    return funcs_.dispatch_input_message(param);
}

void AndroidExternalLib::clear_functions()
{
    funcs_ = {};
}

} // namespace AndroidNativeNS
