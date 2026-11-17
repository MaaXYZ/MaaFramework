#pragma once

#include <string>

#include "InferenceCache.hpp"
#include "MaaUtils/JsonExt.hpp"
#include "OCRSession.h"

MAA_VISION_NS_BEGIN

struct OCRerResult
{
    std::wstring text;
    cv::Rect box {};
    double score = 0.0;

    MEO_JSONIZATION(text, box, score);
};

using OCRCache = InferenceCache<OCRerResult, OCRSessionPtr>;

MAA_VISION_NS_END
