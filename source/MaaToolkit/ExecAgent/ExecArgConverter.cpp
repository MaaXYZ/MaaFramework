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

std::string ExecArgConverter::sync_context_to_arg(MaaSyncContextHandle sync_context)
{
    std::string uuid = std::to_string(reinterpret_cast<uintptr_t>(sync_context));
    sync_contexts_.insert_or_assign(uuid, sync_context);
    return uuid;
}

MaaSyncContextHandle ExecArgConverter::arg_to_sync_context(const std::string& arg) const
{
    auto it = sync_contexts_.find(arg);
    if (it == sync_contexts_.end()) {
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