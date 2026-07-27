#include "ONNXResMgr.h"

#include <array>
#include <filesystem>
#include <limits>
#include <ranges>
#include <unordered_set>

#ifdef _WIN32
#include "MaaUtils/SafeWindows.hpp"
#endif

#include "MLProvider.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "Vision/NeuralNetwork/LabelParser.h"

MAA_RES_NS_BEGIN

ONNXResMgr::ONNXResMgr()
    : memory_info_(Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault))
{
    if (!MAA_VISION_NS::NeuralNetwork::register_builtin_adapters(adapter_registry_)) {
        LogError << "Failed to register built-in neural network adapters";
    }
}

void ONNXResMgr::use_cpu()
{
    LogInfo;
    std::scoped_lock lock(mutex_);

    options_ = { };
    memory_info_ = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);
    invalidate_detector_models();
}

void ONNXResMgr::use_cuda(int device_id)
{
    LogInfo << VAR(device_id);
    std::scoped_lock lock(mutex_);

    Ort::SessionOptions options;
    OrtCUDAProviderOptions cuda_options { };
    cuda_options.device_id = device_id;
    options.AppendExecutionProvider_CUDA(cuda_options);

    // Input tensors are created from std::vector<float> (host memory).
    // Keep CPU memory info here and let ORT move data to CUDA EP internally.
    options_ = std::move(options);
    memory_info_ = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);
    invalidate_detector_models();

    LogInfo << "Using CUDA execution provider with device_id" << device_id;
}

void ONNXResMgr::use_directml(int device_id)
{
    LogInfo << VAR(device_id);
    std::scoped_lock lock(mutex_);

#ifdef MAA_WITH_DML

    Ort::SessionOptions options;
    auto status = OrtSessionOptionsAppendExecutionProvider_DML(options, device_id);
    if (!Ort::Status(status).IsOK()) {
        LogError << "Failed to append DML execution provider with device_id" << device_id;
        return;
    }

    options_ = std::move(options);
    memory_info_ = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);
    invalidate_detector_models();

    LogInfo << "Using DML execution provider with device_id" << device_id;

#else

    LogError << "MaaFW built without DML";

#endif
}

void ONNXResMgr::use_coreml(uint32_t coreml_flag)
{
    LogInfo << VAR(coreml_flag);
    std::scoped_lock lock(mutex_);

#ifdef MAA_WITH_COREML

    Ort::SessionOptions options;
    auto status = OrtSessionOptionsAppendExecutionProvider_CoreML((OrtSessionOptions*)options, coreml_flag);
    if (!Ort::Status(status).IsOK()) {
        LogError << "Failed to append CoreML execution provider";
        return;
    }

    options_ = std::move(options);
    memory_info_ = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);
    invalidate_detector_models();

    LogInfo << "Using CoreML execution provider";

#else

    LogError << "MaaFW built without CoreML";

#endif
}

bool ONNXResMgr::lazy_load_classifier(const std::filesystem::path& path)
{
    LogFunc << VAR(path);

    classifier_roots_.emplace_back(path);

    return true;
}

bool ONNXResMgr::lazy_load_detector(const std::filesystem::path& path)
{
    LogFunc << VAR(path);
    std::scoped_lock lock(mutex_);

    detector_roots_.emplace_back(path);
    detector_models_.clear();

    return true;
}

void ONNXResMgr::clear()
{
    LogFunc;
    std::scoped_lock lock(mutex_);

    classifier_roots_.clear();
    detector_roots_.clear();
    classifiers_.clear();
    detector_models_.clear();
}

std::shared_ptr<Ort::Session> ONNXResMgr::classifier(const std::string& name)
{
    if (auto iter = classifiers_.find(name); iter != classifiers_.end()) {
        return iter->second;
    }

    auto session = load(name, classifier_roots_);
    if (session) {
        classifiers_.emplace(name, session);
    }

    return session;
}

std::shared_ptr<Ort::Session> ONNXResMgr::detector(const std::string& name)
{
    const auto result = detector_model(name);
    return result.package ? result.package->session : nullptr;
}

MAA_VISION_NS::NeuralNetwork::ModelPackageLoadResult ONNXResMgr::detector_model(const std::string& name)
{
    using namespace MAA_VISION_NS::NeuralNetwork;

    std::scoped_lock lock(mutex_);
    std::vector<ModelLayerFiles> layers;
    layers.reserve(detector_roots_.size());
    for (const auto& root : detector_roots_) {
        std::filesystem::path onnx_path = root / MAA_NS::path(name);
        std::filesystem::path descriptor_path = onnx_path;
        descriptor_path.replace_extension(".json");
        std::error_code error;
        auto absolute_path = std::filesystem::absolute(onnx_path, error);
        if (error) {
            return { .error = "failed to resolve detector model path: " + error.message() };
        }
        const bool onnx_exists = std::filesystem::exists(onnx_path, error);
        if (error) {
            return { .error = "failed to inspect detector model path: " + error.message() };
        }
        const bool descriptor_exists = std::filesystem::exists(descriptor_path, error);
        if (error) {
            return { .error = "failed to inspect detector descriptor path: " + error.message() };
        }
        layers.emplace_back(
            ModelLayerFiles {
                .onnx_path = absolute_path.lexically_normal(),
                .onnx_exists = onnx_exists,
                .descriptor_exists = descriptor_exists,
            });
    }

    const auto selected = ModelPathResolver::select(layers);
    if (!selected) {
        return { .error = "detector model not found: " + name };
    }

    const std::string key = ModelPathResolver::cache_key(selected->onnx_path, backend_generation_);
    if (auto iter = detector_models_.find(key); iter != detector_models_.end()) {
        return { .package = iter->second };
    }

    auto result = build_detector_model(*selected);
    if (result.package) {
        detector_models_.emplace(key, result.package);
    }
    return result;
}

const Ort::MemoryInfo& ONNXResMgr::memory_info() const
{
    return memory_info_;
}

std::shared_ptr<Ort::Session> ONNXResMgr::load(const std::string& name, const std::vector<std::filesystem::path>& roots)
{
    LogFunc << VAR(name) << VAR(roots);

    for (const auto& root : roots | std::views::reverse) {
        auto path = root / MAA_NS::path(name);
        if (!std::filesystem::exists(path)) {
            continue;
        }

        LogDebug << VAR(path);
        Ort::Session session(env_, path.c_str(), options_);
        return std::make_shared<Ort::Session>(std::move(session));
    }

    return nullptr;
}

MAA_VISION_NS::NeuralNetwork::ModelPackageLoadResult
    ONNXResMgr::build_detector_model(const MAA_VISION_NS::NeuralNetwork::ModelLayerFiles& layer)
{
    using namespace MAA_VISION_NS::NeuralNetwork;

    try {
        ModelDescriptor descriptor;
        std::unique_ptr<IModelAdapter> adapter;
        std::string error;
        if (layer.descriptor_exists) {
            std::filesystem::path descriptor_path = layer.onnx_path;
            descriptor_path.replace_extension(".json");
            const auto descriptor_json = json::open(descriptor_path, true, false);
            if (!descriptor_json) {
                return { .error = "failed to parse model descriptor: " + descriptor_path.string() };
            }
            if (!ModelDescriptorParser::parse(*descriptor_json, adapter_registry_, descriptor, adapter, error)) {
                return { .error = "invalid model descriptor " + descriptor_path.string() + ": " + error };
            }
        }
        else {
            adapter = adapter_registry_.create("maa_yolo_dense_compat");
            if (!adapter) {
                return { .error = "Maa YOLO Dense compatibility adapter is not registered" };
            }
            if (!adapter->configure({ }, error)) {
                return { .error = "failed to configure Maa YOLO Dense compatibility adapter: " + error };
            }
            descriptor.adapter = "maa_yolo_dense_compat";
            descriptor.preprocess = adapter->default_preprocess();
            descriptor.nms = adapter->default_nms();
            descriptor.nms_threshold = adapter->default_nms_threshold();
        }

        auto session = std::make_shared<Ort::Session>(env_, layer.onnx_path.c_str(), options_);

        Ort::AllocatorWithDefaultOptions allocator;
        std::vector<std::string> input_names;
        std::vector<TensorSpec> input_specs;
        std::vector<std::vector<int64_t>> input_shapes;
        input_names.reserve(session->GetInputCount());
        input_specs.reserve(session->GetInputCount());
        input_shapes.reserve(session->GetInputCount());
        for (size_t index = 0; index < session->GetInputCount(); ++index) {
            const auto input_info = session->GetInputTypeInfo(index).GetTensorTypeAndShapeInfo();
            if (input_info.GetElementType() != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
                return { .error = "detector inputs must be FP32 tensors" };
            }
            input_names.emplace_back(session->GetInputNameAllocated(index, allocator).get());
            input_shapes.emplace_back(input_info.GetShape());
            input_specs.emplace_back(
                TensorSpec {
                    .name = input_names.back(),
                    .shape = input_shapes.back(),
                });
        }
        if (!adapter->validate_inputs(input_specs, error)) {
            return { .error = "detector input contract mismatch: " + error };
        }

        if (!descriptor.input_name.empty() && descriptor.input_name != input_names.front()) {
            return { .error = "descriptor input.name does not match the model input" };
        }
        descriptor.input_name = input_names.front();

        const auto& input_shape = input_shapes.front();
        const int64_t model_height = input_shape[2];
        const int64_t model_width = input_shape[3];
        const cv::Size configured_size = descriptor.preprocess.input_size;
        if (configured_size.width > 0 && configured_size.height > 0) {
            if ((model_width > 0 && model_width != configured_size.width) || (model_height > 0 && model_height != configured_size.height)) {
                return { .error = "descriptor input.size does not match the model input shape" };
            }
        }
        else if (
            model_width > 0 && model_height > 0 && model_width <= std::numeric_limits<int>::max()
            && model_height <= std::numeric_limits<int>::max()) {
            descriptor.preprocess.input_size = {
                static_cast<int>(model_width),
                static_cast<int>(model_height),
            };
        }
        else {
            return { .error = "dynamic detector input H/W requires input.size in the descriptor" };
        }

        std::vector<std::string> available_outputs;
        std::vector<TensorSpec> output_specs;
        available_outputs.reserve(session->GetOutputCount());
        output_specs.reserve(session->GetOutputCount());
        for (size_t index = 0; index < session->GetOutputCount(); ++index) {
            const auto output_info = session->GetOutputTypeInfo(index).GetTensorTypeAndShapeInfo();
            if (output_info.GetElementType() != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
                return { .error = "detector outputs must be FP32 tensors" };
            }
            available_outputs.emplace_back(session->GetOutputNameAllocated(index, allocator).get());
            output_specs.emplace_back(
                TensorSpec {
                    .name = available_outputs.back(),
                    .shape = output_info.GetShape(),
                });
        }
        if (!adapter->validate_outputs(output_specs, error)) {
            return { .error = "detector output contract mismatch: " + error };
        }

        auto output_names = adapter->requested_output_names();
        if (output_names.empty()) {
            if (available_outputs.size() != 1) {
                return { .error = "detector adapter expects exactly one output tensor" };
            }
            output_names = available_outputs;
        }
        else {
            if (output_names.size() != available_outputs.size()) {
                return { .error = "detector output count does not match adapter_config.outputs" };
            }
            for (const auto& output_name : output_names) {
                if (std::ranges::find(available_outputs, output_name) == available_outputs.end()) {
                    return { .error = "configured detector output was not found: " + output_name };
                }
            }
        }

        std::vector<std::string> metadata_labels;
        if (descriptor.labels.empty()) {
            metadata_labels = read_metadata_labels(*session);
        }

        auto package = std::make_shared<ModelPackage>(ModelPackage {
            .onnx_path = layer.onnx_path,
            .session = std::move(session),
            .memory_info = std::make_shared<const Ort::MemoryInfo>(Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault)),
            .adapter = std::shared_ptr<const IModelAdapter>(std::move(adapter)),
            .descriptor = std::move(descriptor),
            .metadata_labels = std::move(metadata_labels),
            .input_names = std::move(input_names),
            .output_names = std::move(output_names),
        });
        return { .package = std::move(package) };
    }
    catch (const Ort::Exception& exception) {
        return { .error = std::string("ONNX Runtime error: ") + exception.what() };
    }
    catch (const std::exception& exception) {
        return { .error = std::string("failed to build detector model package: ") + exception.what() };
    }
}

std::vector<std::string> ONNXResMgr::read_metadata_labels(const Ort::Session& session) const
{
    using MAA_VISION_NS::NeuralNetwork::LabelParser;

    Ort::AllocatorWithDefaultOptions allocator;
    const Ort::ModelMetadata metadata = session.GetModelMetadata();
    constexpr std::array<std::string_view, 4> kMetadataKeys = { "names", "name", "labels", "class_names" };
    for (const auto key : kMetadataKeys) {
        auto raw = metadata.LookupCustomMetadataMapAllocated(key.data(), allocator);
        if (!raw) {
            continue;
        }

        std::vector<std::string> labels;
        std::string error;
        if (LabelParser::parse_metadata(raw.get(), labels, error)) {
            return labels;
        }
        LogWarn << "Failed to parse ONNX label metadata" << VAR(key) << VAR(error);
    }
    return { };
}

void ONNXResMgr::invalidate_detector_models()
{
    ++backend_generation_;
    detector_models_.clear();
}

MAA_RES_NS_END
