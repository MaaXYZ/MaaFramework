#include "TemplateResMgr.h"

#include "Utils/ImageIo.h"
#include "Utils/Logger.h"

MAA_RES_NS_BEGIN

bool TemplateResMgr::lazy_load(const std::filesystem::path& path)
{
    LogFunc << VAR(path);

    roots_.emplace_back(path);
    return true;
}

void TemplateResMgr::clear()
{
    LogFunc;

    roots_.clear();
    image_cahce_.clear();
}

std::vector<cv::Mat> TemplateResMgr::images(const std::vector<std::string>& names)
{
    std::vector<cv::Mat> results;

    for (const auto& name : names) {
        if (auto iter = image_cahce_.find(name); iter != image_cahce_.end()) {
            results.insert(results.end(), iter->second.begin(), iter->second.end());
            continue;
        }

        auto imgs = load(name);
        if (imgs.empty()) {
            continue;
        }
        image_cahce_.emplace(name, imgs);
        results.insert(results.end(), std::make_move_iterator(imgs.begin()), std::make_move_iterator(imgs.end()));
    }

    return results;
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
