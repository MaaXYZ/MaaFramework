#include "ExecArgConverter.h"

#include "MaaFramework/MaaAPI.h"
#include "Utils/ImageIo.h"
#include "Utils/Logger.h"
#include "Utils/Time.hpp"

MAA_TOOLKIT_NS_BEGIN

ExecArgConverter::~ExecArgConverter()
{
    for (const auto& image : images_) {
        std::filesystem::remove(image);
    }
}

std::string ExecArgConverter::context_to_arg(MaaContext* context)
{
    std::string uuid = std::to_string(reinterpret_cast<uintptr_t>(context));
    contexts_.insert_or_assign(uuid, context);
    return uuid;
}

MaaContext* ExecArgConverter::arg_to_context(const std::string& arg) const
{
    auto it = contexts_.find(arg);
    if (it == contexts_.end()) {
        LogError << "sync context not found";
        return nullptr;
    }
    return it->second;
}

std::string ExecArgConverter::image_to_arg(const cv::Mat& image)
{
    auto path = std::filesystem::temp_directory_path() / (format_now_for_filename() + ".png");
    imwrite(path, image);
    images_.push_back(path);
    return path_to_utf8_string(path);
}

MAA_TOOLKIT_NS_END