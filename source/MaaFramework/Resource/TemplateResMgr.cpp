#include "TemplateResMgr.h"

#include "Utils/ImageIo.h"
#include "Utils/Logger.h"

MAA_RES_NS_BEGIN

void TemplateResMgr::add_root(const std::filesystem::path& root)
{
    roots_.emplace_back(root);
}

void TemplateResMgr::set_roots(const std::vector<std::filesystem::path>& roots)
{
    roots_ = roots;
}

bool TemplateResMgr::lazy_load(const std::string& name, const std::vector<std::string>& filenames)
{
    LogDebug << VAR(name) << VAR(filenames);

    template_filenames_.insert_or_assign(name, filenames);
    template_cache_.erase(name);

#ifdef MAA_DEBUG
    const auto& images = get_template_images(name);
    if (images.empty() || MAA_RNS::ranges::any_of(images, [](const auto& image) -> bool { return image.empty(); })) {
        LogError << "image is empty" << VAR(name) << VAR(images);
        return false;
    }
#endif

    return true;
}

void TemplateResMgr::clear()
{
    LogFunc;

    roots_.clear();
    template_filenames_.clear();
    template_cache_.clear();
    template_bank_.clear();
}

const std::vector<cv::Mat>& TemplateResMgr::get_template_images(const std::string& name) const
{
    if (auto templ_iter = template_cache_.find(name); templ_iter != template_cache_.end()) {
        return templ_iter->second;
    }

    return load_images(name);
}

const std::vector<cv::Mat>& TemplateResMgr::load_images(const std::string& name) const
{
    LogFunc << VAR(name);

    auto path_iter = template_filenames_.find(name);
    if (path_iter == template_filenames_.end()) {
        LogError << "Invalid template name" << VAR(name);
        static std::vector<cv::Mat> empty;
        return empty;
    }

    std::vector<cv::Mat> images;
    for (const auto& filename : path_iter->second) {
        cv::Mat templ_mat;
        for (const auto& root : roots_ | MAA_RNS::views::reverse) {
            auto path = root / MAA_NS::path(filename);
            templ_mat = load_single_image(path);
            if (!templ_mat.empty()) {
                break;
            }
        }
        if (templ_mat.empty()) {
            LogError << "template image is empty" << VAR(name) << VAR(filename) << VAR(roots_);
#ifdef MAA_DEBUG
            static std::vector<cv::Mat> empty;
            return empty;
#else
            continue;
#endif
        }
        images.emplace_back(std::move(templ_mat));
    }

    if (images.empty()) {
        LogError << "template list is empty" << VAR(name) << VAR(path_iter->second) << VAR(roots_);
        static std::vector<cv::Mat> empty;
        return empty;
    }

    return template_cache_.emplace(name, std::move(images)).first->second;
}

cv::Mat TemplateResMgr::load_single_image(const std::filesystem::path& path) const
{
    if (auto bank_iter = template_bank_.find(path); bank_iter != template_bank_.end()) {
        LogDebug << "Withdraw image" << VAR(path);
        return bank_iter->second;
    }
    else if (std::filesystem::exists(path)) {
        LogDebug << "Read image" << VAR(path);
        cv::Mat temp = MAA_NS::imread(path);
        template_bank_.emplace(path, temp);
        return temp;
    }

    LogDebug << "Image not found" << VAR(path);
    return {};
}

MAA_RES_NS_END
