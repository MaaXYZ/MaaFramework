#pragma once

#include <filesystem>
#include <memory>

#include <onnxruntime/onnxruntime_cxx_api.h>

#include "Common/Conf.h"
#include "Vision/InferenceSession.h"

MAA_RES_NS_BEGIN

class OrtSession : public MAA_VISION_NS::InferenceSession
{
public:
    // returns nullptr on failure
    static std::shared_ptr<OrtSession> create(
        Ort::Env& env,
        const std::filesystem::path& path,
        const Ort::SessionOptions& options,
        const Ort::MemoryInfo& memory_info);

    virtual ~OrtSession() override = default;

public:
    virtual MAA_VISION_NS::InferenceEngine engine() const override { return MAA_VISION_NS::InferenceEngine::ONNXRuntime; }

    virtual const std::vector<int64_t>& input_shape() const override { return input_shape_; }

    virtual std::optional<MAA_VISION_NS::InferenceOutput> run(const std::vector<float>& input) const override;

    virtual const std::unordered_map<std::string, std::string>& metadata() const override { return metadata_; }

private:
    OrtSession(Ort::Session session, Ort::MemoryInfo memory_info);

    mutable Ort::Session session_;
    Ort::MemoryInfo memory_info_;

    std::string input_name_;
    std::string output_name_;
    std::vector<int64_t> input_shape_;
    std::unordered_map<std::string, std::string> metadata_;
};

MAA_RES_NS_END
