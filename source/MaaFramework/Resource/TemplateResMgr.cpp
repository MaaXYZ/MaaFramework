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
    images_.clear();
}

std::shared_ptr<TemplateResMgr::Image> TemplateResMgr::image(const std::string& name)
{
    if (auto iter = images_.find(name); iter != images_.end()) {
        return iter->second;
    }

    auto img = load(name);
    if (img) {
        images_.emplace(name, img);
    }

    return img;
}

std::shared_ptr<TemplateResMgr::Image> TemplateResMgr::load(const std::string& name)
{
    LogFunc << VAR(name) << VAR(roots_);

    for (const auto& root : roots_ | std::views::reverse) {
        auto path = root / MAA_NS::path(name);
        if (!std::filesystem::exists(path)) {
            continue;
        }

        LogDebug << VAR(path);
        cv::Mat image = MAA_NS::imread(path);
        if (image.empty()) {
            LogError << "Failed to load image: " << path;
            return nullptr;
        }
        return std::make_shared<Image>(std::move(image));
    }

    return nullptr;
}

MAA_RES_NS_END
