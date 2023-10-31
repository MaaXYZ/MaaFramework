#include "CarouselImage.h"

#include "Utils/ImageIo.h"
#include "Utils/Logger.h"
#include "Utils/StringMisc.hpp"

MAA_DBG_CTRL_UNIT_NS_BEGIN

std::string CarouselImage::uuid() const
{
    return path_to_utf8_string(path_);
}

cv::Size CarouselImage::resolution() const
{
    return resolution_;
}

bool CarouselImage::connect()
{
    LogInfo << VAR(path_);

    images_.clear();
    image_index_ = 0;

    if (!std::filesystem::exists(path_)) {
        LogError << VAR(path_) << "not exits";
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

bool CarouselImage::press_key(int key)
{
    std::ignore = key;
    return true;
}

std::optional<cv::Mat> CarouselImage::screencap()
{
    if (images_.empty()) {
        LogError << "no image" << VAR(path_);
        return std::nullopt;
    }

    if (image_index_ >= images_.size()) {
        image_index_ = 0;
    }

    return images_.at(image_index_++);
}

MAA_DBG_CTRL_UNIT_NS_END
