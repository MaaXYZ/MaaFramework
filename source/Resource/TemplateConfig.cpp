#include "TemplateConfig.h"

#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"

MAA_RES_NS_BEGIN

bool TemplateConfig::lazy_load(const std::string& name, const std::filesystem::path& path)
{
    LogTrace << VAR(name) << VAR(path);

    if (!std::filesystem::exists(path)) {
        LogError << "not exists" << VAR(path);
        return false;
    }

    if (auto old_path_iter = template_paths_.find(name);
        old_path_iter != template_paths_.end() && path == old_path_iter->second) {
        LogDebug << "same path, ignore" << VAR(path);
        return true;
    }
    template_paths_.insert_or_assign(name, path);
    template_cache_.erase(name);

#ifdef MAA_DEBUG
    cv::Mat image = get_template_image(name);
    if (image.empty()) {
        LogError << "image is empty" << VAR(image);
        return false;
    }
#endif

    return true;
}

bool TemplateConfig::lazy_load(const std::map<std::string, std::filesystem::path>& path_map)
{
    LogFunc << VAR(path_map.size());

    for (const auto& [name, path] : path_map) {
        bool ret = lazy_load(name, path);
        if (!ret) {
            return false;
        }
    }
    return true;
}

void TemplateConfig::clear()
{
    LogFunc;

    template_paths_.clear();
    template_cache_.clear();
    template_bank_.clear();
}

const cv::Mat& TemplateConfig::get_template_image(const std::string& name) const
{
    if (auto templ_iter = template_cache_.find(name); templ_iter != template_cache_.end()) {
        return templ_iter->second;
    }

    auto path_iter = template_paths_.find(name);
    if (path_iter == template_paths_.end()) {
        LogError << "Invalid template name" << VAR(name);
        static cv::Mat empty;
        return empty;
    }
    const auto& path = path_iter->second;

    cv::Mat image;
    auto bank_iter = template_bank_.find(path);
    if (bank_iter != template_bank_.end()) {
        image = bank_iter->second;
        LogTrace << "Withdraw image" << VAR(name) << VAR(path) << VAR(image);
    }
    else {
        image = imread(path);
        LogTrace << "Read image" << VAR(name) << VAR(path) << VAR(image);
        template_bank_.emplace(name, image);
    }
    return template_cache_.emplace(name, std::move(image)).first->second;
}

MAA_RES_NS_END
