#include "DbgController.h"

#include <algorithm>
#include <cctype>
#include <string_view>
#include <unordered_set>

#include "MaaUtils/ImageIo.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"

MAA_CTRL_UNIT_NS_BEGIN

namespace
{

bool is_image_extension(std::string_view ext)
{
    static const std::unordered_set<std::string> kExts = {
        ".png", ".jpg", ".jpeg", ".bmp", ".webp", ".tif", ".tiff",
    };
    std::string lower(ext.begin(), ext.end());
    for (char& c : lower) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return kExts.contains(lower);
}

bool is_image_path(const std::filesystem::path& p)
{
    return is_image_extension(MAA_NS::path_to_utf8_string(p.extension()));
}

} // namespace

DbgController::DbgController(std::filesystem::path path)
    : path_(std::move(path))
{
    LogFunc << VAR(path_);
}

bool DbgController::connect()
{
    LogFunc << VAR(path_);

    image_paths_.clear();
    image_index_ = 0;
    connected_ = false;

    std::error_code ec;
    if (!std::filesystem::exists(path_, ec)) {
        LogError << "path does not exist" << VAR(path_);
        return false;
    }

    if (std::filesystem::is_regular_file(path_, ec)) {
        if (is_image_path(path_)) {
            image_paths_.push_back(path_);
        }
    }
    else if (std::filesystem::is_directory(path_, ec)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path_, ec)) {
            if (ec) {
                LogWarn << "recursive_directory_iterator error" << VAR(ec.message());
                break;
            }
            if (!entry.is_regular_file()) {
                continue;
            }
            if (is_image_path(entry.path())) {
                image_paths_.push_back(entry.path());
            }
        }
    }

    std::ranges::sort(image_paths_, [](const std::filesystem::path& a, const std::filesystem::path& b) {
        return MAA_NS::path_to_utf8_string(a) < MAA_NS::path_to_utf8_string(b);
    });

    if (image_paths_.empty()) {
        LogError << "no image files found" << VAR(path_);
        return false;
    }

    connected_ = true;
    return true;
}

bool DbgController::connected() const
{
    return connected_;
}

bool DbgController::request_uuid(std::string& uuid)
{
    uuid = MAA_NS::path_to_utf8_string(path_);
    return true;
}

MaaControllerFeature DbgController::get_features() const
{
    return MaaControllerFeature_None;
}

bool DbgController::start_app(const std::string& /*intent*/)
{
    return true;
}

bool DbgController::stop_app(const std::string& /*intent*/)
{
    return true;
}

bool DbgController::screencap(cv::Mat& image)
{
    if (!connected_ || image_paths_.empty()) {
        LogError << "not connected or no images";
        return false;
    }

    const size_t idx = image_index_ % image_paths_.size();
    image = MAA_NS::imread(image_paths_[idx]);
    image_index_ = (image_index_ + 1) % image_paths_.size();

    if (image.empty()) {
        LogError << "failed to load image" << VAR(image_paths_[idx]);
        return false;
    }

    return true;
}

bool DbgController::click(int /*x*/, int /*y*/)
{
    return true;
}

bool DbgController::swipe(int /*x1*/, int /*y1*/, int /*x2*/, int /*y2*/, int /*duration*/)
{
    return true;
}

bool DbgController::touch_down(int /*contact*/, int /*x*/, int /*y*/, int /*pressure*/)
{
    return true;
}

bool DbgController::touch_move(int /*contact*/, int /*x*/, int /*y*/, int /*pressure*/)
{
    return true;
}

bool DbgController::touch_up(int /*contact*/)
{
    return true;
}

bool DbgController::click_key(int /*key*/)
{
    return true;
}

bool DbgController::input_text(const std::string& /*text*/)
{
    return true;
}

bool DbgController::key_down(int /*key*/)
{
    return true;
}

bool DbgController::key_up(int /*key*/)
{
    return true;
}

bool DbgController::inactive()
{
    return true;
}

json::object DbgController::get_info() const
{
    json::object info;
    info["type"] = "dbg";
    info["path"] = MAA_NS::path_to_utf8_string(path_);
    info["image_count"] = static_cast<int64_t>(image_paths_.size());
    info["image_index"] = static_cast<int64_t>(image_index_);
    return info;
}

MAA_CTRL_UNIT_NS_END
