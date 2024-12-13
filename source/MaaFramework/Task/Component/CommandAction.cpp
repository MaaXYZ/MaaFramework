#include "CommandAction.h"

#include <functional>

#include "Utils/Codec.h"
#include "Utils/IOStream/BoostIO.hpp"
#include "Utils/Logger.h"
#include "Utils/Platform.h"
#include "Utils/ImageIo.h"

MAA_TASK_NS_BEGIN

CommandAction::~CommandAction()
{
    for (const auto& p : cached_images_) {
        if (!std::filesystem::exists(p)) {
            continue;
        }
        LogTrace << "remove" << VAR(p);
        std::filesystem::remove(p);
    }
}

bool CommandAction::run(const MAA_RES_NS::Action::CommandParam& command, const Runtime& runtime)
{
    LogFunc << VAR(command.exec) << VAR(command.args) << VAR(command.detach);

    static std::unordered_map<std::string, std::function<std::string(const Runtime&)>> kArgvReplacement = {
        { "Entry", std::bind(&CommandAction::gen_entry_name, this, std::placeholders::_1) },
        { "Node", std::bind(&CommandAction::gen_node_name, this, std::placeholders::_1) },
        { "Image", std::bind(&CommandAction::gen_image_path, this, std::placeholders::_1) },
        { "Box", std::bind(&CommandAction::gen_box, this, std::placeholders::_1) },
    };

    std::filesystem::path exec = path(command.exec);

    std::vector<os_string> args;
    for (const std::string& arg : command.args) {
        auto iter = kArgvReplacement.find(arg);

        std::string dst;
        if (iter == kArgvReplacement.end()) {
            dst = arg;
        }
        else {
            dst = iter->second(runtime);
        }
#ifdef _WIN32
        args.emplace_back(to_u16(dst));
#else
        args.emplace_back(dst);
#endif
    }

    LogInfo << VAR(exec) << VAR(args);
    boost::process::child child(exec, args);

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

std::string CommandAction::gen_entry_name(const Runtime& runtime)
{
    return runtime.entry;
}

std::string CommandAction::gen_node_name(const Runtime& runtime)
{
    return runtime.node;
}

std::string CommandAction::gen_image_path(const Runtime& runtime)
{
    auto dst_path = std::filesystem::temp_directory_path() / (format_now_for_filename() + ".png");
    cached_images_.emplace_back(dst_path);
    imwrite(dst_path, runtime.image);
    return path_to_utf8_string(dst_path);
}

std::string CommandAction::gen_box(const Runtime& runtime)
{
    return std::format("[{},{},{},{}]", runtime.box.x, runtime.box.y, runtime.box.width, runtime.box.height);
}

MAA_TASK_NS_END
