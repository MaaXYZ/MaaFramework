#pragma once

#include <vector>

#include "Instance/InstanceInternalAPI.hpp"
#include "MaaFramework/MaaDef.h"
#include "Utils/JsonExt.hpp"
#include "VisionBase.h"
#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

class CustomRecognizer : public VisionBase
{
    struct Result
    {
        cv::Rect box {};
        json::value detail;

        MEO_JSONIZATION(box, detail);
    };

public:
    CustomRecognizer(
        cv::Mat image,
        CustomRecognizerParam param,
        CustomRecognizerSession session,
        InstanceInternalAPI* inst,
        std::string name = "");

    bool ret() const { return ret_; }

    const Result& result() const& { return result_; }

    Result result() && { return std::move(result_); }

private:
    void analyze();

private:
    const CustomRecognizerParam param_;
    CustomRecognizerSession session_;
    InstanceInternalAPI* inst_ = nullptr;

private:
    bool ret_ = false;
    Result result_;
};

MAA_VISION_NS_END
