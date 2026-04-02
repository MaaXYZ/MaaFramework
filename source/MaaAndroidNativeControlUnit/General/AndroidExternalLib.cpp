#include "AndroidExternalLib.h"

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

    bool ok = get_required_function<AndroidNativeNS::GetLockedPixelsSignature>(get_locked_pixels_func_name_, funcs.get_locked_pixels);
    ok = ok && get_required_function<AndroidNativeNS::UnlockPixelsSignature>(unlock_pixels_func_name_, funcs.unlock_pixels);
    ok = ok
         && get_required_function<AndroidNativeNS::DispatchInputMessageSignature>(
             dispatch_input_message_func_name_,
             funcs.dispatch_input_message);

    if (!ok) {
        unload_library();
        return std::nullopt;
    }

    return funcs;
}

MAA_NS_END
