#pragma once

#include "API/MaaTypes.h"
#include "Conf/Conf.h"
#include "ListBuffer.hpp"

MAA_SUPPRESS_CV_WARNINGS_BEGIN
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
MAA_SUPPRESS_CV_WARNINGS_END

MAA_NS_BEGIN

class ImageBuffer : public MaaImageBuffer
{
public:
    ImageBuffer() = default;

    ImageBuffer(cv::Mat image)
        : image_(std::move(image))
    {
    }

    virtual ~ImageBuffer() override = default;

    virtual bool empty() const override { return image_.empty(); }

    virtual void clear() override
    {
        image_.release();
        encoded_.clear();
    }

    virtual void* raw_data() const override { return image_.data; }

    virtual int32_t width() const override { return image_.cols; }

    virtual int32_t height() const override { return image_.rows; }

    virtual int32_t type() const override { return image_.type(); }

    virtual uint8_t* encoded() override
    {
        encode();
        return encoded_.data();
    }

    virtual size_t encoded_size() override
    {
        encode();
        return encoded_.size();
    }

    virtual const cv::Mat& get() const override { return image_; }

    virtual void set(cv::Mat image) override
    {
        dirty_ = true;
        image_ = image.clone();
    }

private:
    void encode()
    {
        if (!dirty_) {
            return;
        }

        if (image_.empty()) {
            encoded_.clear();
        }
        else {
            cv::imencode(".png", image_, encoded_);
        }
        dirty_ = false;
    }

    cv::Mat image_;
    bool dirty_ = true;
    std::vector<uint8_t> encoded_;
};

MAA_NS_END

struct MaaImageListBuffer : public MAA_NS::ListBuffer<MAA_NS::ImageBuffer>
{
};