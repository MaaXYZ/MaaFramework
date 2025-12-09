#include "CarouselImage.h"

#include "MaaUtils/ImageIo.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/StringMisc.hpp"

MAA_CTRL_UNIT_NS_BEGIN

bool CarouselImage::connect()
{
    LogInfo << VAR(path_);

    images_.clear();
    image_index_ = 0;

    if (!std::filesystem::exists(path_)) {
        LogError << VAR(path_) << "not exists";
        return false;
    }

    auto try_emplace_image = [&](const std::filesystem::path& path) {
        if (!std::filesystem::is_regular_file(path)) {
            return false;
        }

        auto image = imread(path);
        if (image.empty()) {
            return false;
        }

        images_.emplace_back(std::move(image));
        return true;
    };

    if (std::filesystem::is_directory(path_)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path_)) {
            try_emplace_image(entry.path());
        }
    }
    else {
        try_emplace_image(path_);
    }

    LogInfo << VAR(images_.size());

    if (images_.empty()) {
        LogError << "no image" << VAR(path_);
        return false;
    }

    resolution_ = images_.front().size();
    return true;
}

bool CarouselImage::request_uuid(std::string& uuid)
{
    uuid = path_to_utf8_string(path_);
    return true;
}

MaaControllerFeature CarouselImage::get_features() const
{
    return MaaControllerFeature_None;
}

bool CarouselImage::start_app(const std::string& intent)
{
    std::ignore = intent;
    return true;
}

bool CarouselImage::stop_app(const std::string& intent)
{
    std::ignore = intent;
    return true;
}

bool CarouselImage::screencap(cv::Mat& image)
{
    if (images_.empty()) {
        LogError << "no image" << VAR(path_);
        return false;
    }

    if (image_index_ >= images_.size()) {
        image_index_ = 0;
    }

    image = images_.at(image_index_++);
    return true;
}

bool CarouselImage::click(int x, int y)
{
    std::ignore = x;
    std::ignore = y;
    return true;
}

bool CarouselImage::swipe(int x1, int y1, int x2, int y2, int duration)
{
    std::ignore = x1;
    std::ignore = y1;
    std::ignore = x2;
    std::ignore = y2;
    std::ignore = duration;

    return true;
}

bool CarouselImage::touch_down(int contact, int x, int y, int pressure)
{
    std::ignore = contact;
    std::ignore = x;
    std::ignore = y;
    std::ignore = pressure;

    return true;
}

bool CarouselImage::touch_move(int contact, int x, int y, int pressure)
{
    std::ignore = contact;
    std::ignore = x;
    std::ignore = y;
    std::ignore = pressure;

    return true;
}

bool CarouselImage::touch_up(int contact)
{
    std::ignore = contact;
    return true;
}

bool CarouselImage::click_key(int key)
{
    std::ignore = key;
    return true;
}

bool CarouselImage::input_text(const std::string& text)
{
    std::ignore = text;
    return true;
}

bool CarouselImage::key_down(int key)
{
    std::ignore = key;
    return true;
}

bool CarouselImage::key_up(int key)
{
    std::ignore = key;
    return true;
}

bool CarouselImage::scroll(int dx, int dy)
{
    std::ignore = dx;
    std::ignore = dy;
    return true;
}

MAA_CTRL_UNIT_NS_END
