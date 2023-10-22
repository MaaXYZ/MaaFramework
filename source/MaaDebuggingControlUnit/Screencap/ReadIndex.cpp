#include "ControlUnitMgr.h"

#include "ReadIndex.h"
#include "Utils/ImageIo.h"
#include "Utils/Logger.h"

MAA_DBG_CTRL_UNIT_NS_BEGIN

ReadIndex::ReadIndex(std::filesystem::path indexfile) : indexfile_(std::move(indexfile)) {}

bool ReadIndex::init(int swidth, int sheight)
{
    LogInfo << VAR(indexfile_) << VAR(swidth) << VAR(sheight);

    swidth_ = swidth;
    sheight_ = sheight;
    filepaths_.clear();
    index_ = 0;

    if (!std::filesystem::exists(indexfile_)) {
        LogError << "indexfile not exist" << VAR(indexfile_);
        return false;
    }

    std::ifstream ifs(indexfile_);
    if (!ifs.is_open()) {
        LogError << "open indexfile failed" << VAR(indexfile_);
        return false;
    }

    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) {
            continue;
        }
        std::istringstream iss(line);
        std::filesystem::path image_path;
        if (image_path.is_absolute()) {
            image_path = line;
        }
        else {
            image_path = indexfile_.parent_path() / line;
        }
        if (!std::filesystem::exists(image_path)) {
            LogError << "image not exist" << VAR(image_path);
            continue;
        }
        filepaths_.emplace_back(std::move(image_path));
    }

    return !filepaths_.empty();
}

std::optional<cv::Mat> ReadIndex::screencap()
{
    if (index_ >= filepaths_.size()) {
        LogInfo << "index_ >= filepaths_.size(), reset index_ to 0";
        index_ = 0;
    }

    auto image_path = filepaths_[index_++];
    LogInfo << VAR(image_path);

    LogInfo << "read image" << VAR(image_path);
    auto image = imread(image_path);
    if (image.empty()) {
        LogError << "read image failed" << VAR(image_path);
        return std::nullopt;
    }

    cv::resize(image, image, cv::Size(swidth_, sheight_));
    return image;
}

MAA_DBG_CTRL_UNIT_NS_END
