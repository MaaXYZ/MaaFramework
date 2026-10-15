#pragma once

#include "NeuralNetworkClassifier.h"
#include "NeuralNetworkDetector.h"

MAA_VISION_NS_BEGIN

struct NeuralNetworkCache
{
    InferenceCache<NeuralNetworkClassifierResult> classifiers;
    InferenceCache<NeuralNetworkDetectorResult> detectors;
};

MAA_VISION_NS_END
