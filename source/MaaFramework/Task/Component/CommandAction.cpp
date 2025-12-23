#include "CommandAction.h"

#include <functional>

#include "MaaUtils/Encoding.h"
#include "MaaUtils/IOStream/BoostIO.hpp"
#include "MaaUtils/ImageIo.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "MaaUtils/Runtime.h"
#include "MaaUtils/StringMisc.hpp"

MAA_TASK_NS_BEGIN

TempFileHolder::~TempFileHolder()
{
    LogFunc;

    for (const auto& p : files_) {
        if (!std::filesystem::exists(p)) {
            continue;
        }
        LogTrace << "remove" << VAR(p);
        std::error_code ec;
        std::filesystem::remove(p, ec);
    }
}

void TempFileHolder::emplace(const std::filesystem::path& p)
{
    LogTrace << p;
    files_.emplace_back(p);
}

bool CommandAction::run(const MAA_RES_NS::Action::CommandParam& command, const Runtime& runtime)
{
    LogFunc << VAR(command.exec) << VAR(command.args) << VAR(command.detach);

    auto gen_runtime = [&](const std::string& src) -> std::string {
        static std::unordered_map<std::string, std::function<std::string(const Runtime&)>> kArgvReplacement = {
            { "{ENTRY}", std::bind(&CommandAction::get_entry_name, this, std::placeholders::_1) },
            { "{NODE}", std::bind(&CommandAction::get_node_name, this, std::placeholders::_1) },
            { "{IMAGE}", std::bind(&CommandAction::get_image_path, this, std::placeholders::_1) },
            { "{BOX}", std::bind(&CommandAction::get_box, this, std::placeholders::_1) },
            { "{LIBRARY_DIR}", std::bind(&CommandAction::get_library_dir, this, std::placeholders::_1) },
            { "{RESOURCE_DIR}", std::bind(&CommandAction::get_resource_dir, this, std::placeholders::_1) },
        };

        std::string dst = src;
        for (const auto& [key, func] : kArgvReplacement) {
            if (src.find(key) == std::string::npos) {
                continue;
            }
            dst = string_replace_all(dst, key, func(runtime));
        }
        return dst;
    };

    std::string conv_exec = gen_runtime(command.exec);
    std::filesystem::path exec = boost::process::search_path(path(conv_exec));
    if (!std::filesystem::exists(exec)) {
        LogError << "exec not exists" << VAR(command.exec) << VAR(conv_exec) << VAR(exec);
        return false;
    }

    std::vector<os_string> args;
    for (const std::string& arg : command.args) {
        std::string dst = gen_runtime(arg);

#ifdef _WIN32
        args.emplace_back(to_u16(dst));
#else
        args.emplace_back(dst);
#endif
    }

    LogInfo << VAR(exec) << VAR(args);
    boost::process::child child(exec, args, boost::process::start_dir = exec.parent_path());

    if (!child.joinable()) {
        LogError << "child is not joinable";
        return false;
    }

    if (command.detach) {
        child.detach();
    }
    else {
        child.join();
    }
    return true;
}

std::string CommandAction::get_entry_name(const Runtime& runtime)
{
    return runtime.entry;
}

std::string CommandAction::get_node_name(const Runtime& runtime)
{
    return runtime.node;
}

std::string CommandAction::get_image_path(const Runtime& runtime)
{
    if (!image_path_.empty()) {
        return image_path_;
    }

    auto dst_path = std::filesystem::temp_directory_path() / (format_now_for_filename() + ".png");
    TempFileHolder::get_instance().emplace(dst_path);
    imwrite(dst_path, runtime.image);
    image_path_ = path_to_utf8_string(dst_path);
    return image_path_;
}

std::string CommandAction::get_box(const Runtime& runtime)
{
    return std::format("[{},{},{},{}]", runtime.box.x, runtime.box.y, runtime.box.width, runtime.box.height);
}

std::string CommandAction::get_library_dir(const Runtime& runtime)
{
    std::ignore = runtime;

    return path_to_utf8_string(library_dir());
}

std::string CommandAction::get_resource_dir(const Runtime& runtime)
{
    if (runtime.resource_paths.empty()) {
        LogWarn << "no resource";
        return {};
    }

    const auto& p = runtime.resource_paths.back();
    if (std::filesystem::is_regular_file(p)) {
        return path_to_utf8_string(p.parent_path());
    }
    return path_to_utf8_string(p);
}

MAA_TASK_NS_END
