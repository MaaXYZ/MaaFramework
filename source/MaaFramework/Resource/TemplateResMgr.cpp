#include "TemplateResMgr.h"

#include "MaaUtils/ImageIo.h"
#include "MaaUtils/Logger.h"

MAA_RES_NS_BEGIN

bool TemplateResMgr::lazy_load(const std::filesystem::path& path)
{
    LogFunc << VAR(path);

    roots_.emplace_back(path);
    return true;
}

bool TemplateResMgr::load_file(const std::filesystem::path& path)
{
    LogFunc << VAR(path);

    if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
        LogError << "path not exists or not a file" << VAR(path);
        return false;
    }

    cv::Mat image = MAA_NS::imread(path);
    if (image.empty()) {
        LogError << "Failed to load image:" << path;
        return false;
    }

    auto name = path_to_utf8_string(path.filename());
    image_cache_[name] = { std::move(image) };
    return true;
}

void TemplateResMgr::clear()
{
    LogFunc;

    roots_.clear();
    image_cache_.clear();
}

std::vector<cv::Mat> TemplateResMgr::get_image(const std::string& name)
{
    if (auto iter = image_cache_.find(name); iter != image_cache_.end()) {
        return iter->second;
    }

    auto imgs = load(name);
    if (imgs.empty()) {
        return {};
    }
    image_cache_.emplace(name, imgs);
    return imgs;
}

void TemplateResMgr::set_image(const std::string& name, const cv::Mat& image)
{
    image_cache_[name] = { image };
}

std::vector<cv::Mat> TemplateResMgr::load(const std::string& name)
{
    LogFunc << VAR(name) << VAR(roots_);

    auto load_regular_image = [&](const std::filesystem::path& path) -> cv::Mat {
        if (!std::filesystem::exists(path)) {
            LogError << "File does not exist:" << path;
            return {};
        }
        LogDebug << VAR(path);

        cv::Mat image = MAA_NS::imread(path);

        if (image.empty()) {
            LogError << "Failed to load image:" << path;
            return {};
        }
        return image;
    };

    std::vector<cv::Mat> results;

    for (const auto& root : roots_ | std::views::reverse) {
        auto path = root / MAA_NS::path(name);
        if (!std::filesystem::exists(path)) {
            continue;
        }
        LogDebug << VAR(path);

        if (std::filesystem::is_regular_file(path)) {
            cv::Mat image = load_regular_image(path);
            if (image.empty()) {
                continue;
            }
            results.emplace_back(std::move(image));
        }
        else if (std::filesystem::is_directory(path)) {
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                if (!entry.is_regular_file()) {
                    continue;
                }
                cv::Mat image = load_regular_image(entry.path());
                if (image.empty()) {
                    continue;
                }
                results.emplace_back(std::move(image));
            }
        }
        else {
            LogError << "Path is neither a file nor a directory:" << path;
            continue;
        }
    }

    return results;
}

MAA_RES_NS_END
