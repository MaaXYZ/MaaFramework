#include "TemplateConfig.h"

#include "MaaUtils/Logger.hpp"
#include "Utils/ImageIo.hpp"

MAA_RES_NS_BEGIN

bool TemplateConfig::lazy_load(const std::string& name, const std::vector<std::filesystem::path>& paths)
{
    LogTrace << VAR(name) << VAR(paths);

    if (!ranges::all_of(paths, [](const auto& path) -> bool { return std::filesystem::exists(path); })) {
        LogError << "not exists" << VAR(paths);
        return false;
    }

    if (auto old_path_iter = template_paths_.find(name);
        old_path_iter != template_paths_.end() && paths == old_path_iter->second) {
        LogDebug << "same paths, ignore" << VAR(paths);
        return true;
    }
    template_paths_.insert_or_assign(name, paths);
    template_cache_.erase(name);

#ifdef MAA_DEBUG
    const auto& images = get_template_images(name);
    if (ranges::any_of(images, [](const auto& image) -> bool { return image.empty(); })) {
        LogError << "image is empty" << VAR(images);
        return false;
    }
#endif

    return true;
}

void TemplateConfig::clear()
{
    LogFunc;

    template_paths_.clear();
    template_cache_.clear();
    template_bank_.clear();
}

const std::vector<cv::Mat>& TemplateConfig::get_template_images(const std::string& name) const
{
    if (auto templ_iter = template_cache_.find(name); templ_iter != template_cache_.end()) {
        return templ_iter->second;
    }

    LogFunc << "Load Templ" << VAR(name);

    auto path_iter = template_paths_.find(name);
    if (path_iter == template_paths_.end()) {
        LogError << "Invalid template name" << VAR(name);
        static std::vector<cv::Mat> empty;
        return empty;
    }
    const auto& paths = path_iter->second;

    std::vector<cv::Mat> images;
    for (const auto& path : paths) {
        auto bank_iter = template_bank_.find(path);

        if (bank_iter != template_bank_.end()) {
            images.emplace_back(bank_iter->second);
            LogTrace << "Withdraw image" << VAR(name) << VAR(path);
        }
        else {
            auto& image = images.emplace_back(imread(path));
            LogTrace << "Read image" << VAR(name) << VAR(path);
            template_bank_.emplace(name, image);
        }
    }
    return template_cache_.emplace(name, std::move(images)).first->second;
}

MAA_RES_NS_END
