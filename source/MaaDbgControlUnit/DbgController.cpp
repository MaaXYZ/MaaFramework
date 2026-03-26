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

    images_.clear();
    image_index_ = 0;
    connected_ = false;

    std::error_code ec;
    if (!std::filesystem::exists(path_, ec)) {
        LogError << "path does not exist" << VAR(path_);
        return false;
    }

    std::vector<std::filesystem::path> paths;

    if (std::filesystem::is_regular_file(path_, ec)) {
        if (is_image_path(path_)) {
            paths.push_back(path_);
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
                paths.push_back(entry.path());
            }
        }
    }

    std::ranges::sort(paths, [](const std::filesystem::path& a, const std::filesystem::path& b) {
        return MAA_NS::path_to_utf8_string(a) < MAA_NS::path_to_utf8_string(b);
    });

    for (const auto& p : paths) {
        cv::Mat img = MAA_NS::imread(p);
        if (!img.empty()) {
            images_.push_back(std::move(img));
        }
    }

    if (images_.empty()) {
        LogError << "no loadable images" << VAR(path_);
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
    if (!connected_ || images_.empty()) {
        LogError << "not connected or no images";
        return false;
    }

    const size_t idx = image_index_ % images_.size();
    images_[idx].copyTo(image);
    image_index_ = (image_index_ + 1) % images_.size();
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
    info["image_count"] = static_cast<int64_t>(images_.size());
    info["image_index"] = static_cast<int64_t>(image_index_);
    return info;
}

MAA_CTRL_UNIT_NS_END
