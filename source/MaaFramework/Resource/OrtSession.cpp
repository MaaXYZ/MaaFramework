#include "OrtSession.h"

#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"

MAA_RES_NS_BEGIN

std::shared_ptr<OrtSession> OrtSession::create(
    Ort::Env& env,
    const std::filesystem::path& path,
    const Ort::SessionOptions& options,
    const Ort::MemoryInfo& memory_info,
    const InferenceContext& context)
{
    LogFunc << VAR(path);

    try {
        return context.make_shared<OrtSession>(env, path, options, memory_info, context);
    }
    catch (const Ort::Exception& e) {
        LogError << "Failed to create onnxruntime session" << VAR(path) << VAR(e.what());
        return nullptr;
    }
}

OrtSession::OrtSession(
    Ort::Env& env,
    const std::filesystem::path& path,
    const Ort::SessionOptions& options,
    const Ort::MemoryInfo& memory_info,
    const InferenceContext& context)
    : context_(context)
    , session_(env, path.c_str(), options)
    // Ort::MemoryInfo is not copyable, rebuild an equivalent one.
    , memory_info_(
          memory_info.GetAllocatorName().c_str(),
          memory_info.GetAllocatorType(),
          memory_info.GetDeviceId(),
          memory_info.GetMemoryType())
{
    Ort::AllocatorWithDefaultOptions allocator;

    input_name_ = session_.GetInputNameAllocated(0, allocator).get();
    output_name_ = session_.GetOutputNameAllocated(0, allocator).get();
    input_shape_ = session_.GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape();

    Ort::ModelMetadata model_metadata = session_.GetModelMetadata();
    for (const auto& key : model_metadata.GetCustomMetadataMapKeysAllocated(allocator)) {
        auto value = model_metadata.LookupCustomMetadataMapAllocated(key.get(), allocator);
        if (!value) {
            continue;
        }
        metadata_.emplace(key.get(), value.get());
    }

    LogDebug << VAR(input_name_) << VAR(output_name_) << VAR(input_shape_);
}

std::optional<MAA_VISION_NS::InferenceOutput> OrtSession::run(const std::vector<float>& input) const
{
    auto lock = context_.lock();

    try {
        // CreateTensor does not modify the buffer
        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            memory_info_,
            const_cast<float*>(input.data()),
            input.size(),
            input_shape_.data(),
            input_shape_.size());

        const char* input_names[] = { input_name_.c_str() };
        const char* output_names[] = { output_name_.c_str() };

        Ort::RunOptions run_options;
        auto output_tensor = session_.Run(run_options, input_names, &input_tensor, 1, output_names, 1);

        auto info = output_tensor[0].GetTensorTypeAndShapeInfo();
        const float* raw_output = output_tensor[0].GetTensorData<float>();

        MAA_VISION_NS::InferenceOutput output;
        output.shape = info.GetShape();
        output.data.assign(raw_output, raw_output + info.GetElementCount());
        return output;
    }
    catch (const Ort::Exception& e) {
        LogError << "onnxruntime run failed" << VAR(e.what()) << VAR(input_shape_) << VAR(input.size());
        return std::nullopt;
    }
}

MAA_RES_NS_END
