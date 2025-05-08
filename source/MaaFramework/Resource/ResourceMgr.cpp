#include "ResourceMgr.h"

#include <tuple>

#include "MLProvider.h"
#include "MaaFramework/MaaMsg.h"
#include "Utils/GpuOption.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

MAA_RES_NS_BEGIN

ResourceMgr::ResourceMgr(MaaNotificationCallback notify, void* notify_trans_arg)
    : notifier_(notify, notify_trans_arg)
{
    LogFunc << VAR_VOIDP(notify) << VAR_VOIDP(notify_trans_arg);

    res_loader_ = std::make_unique<AsyncRunner<std::filesystem::path>>(
        std::bind(&ResourceMgr::run_load, this, std::placeholders::_1, std::placeholders::_2));
}

ResourceMgr::~ResourceMgr()
{
    LogFunc;

    if (res_loader_) {
        res_loader_->wait_all();
    }
}

bool ResourceMgr::set_option(MaaResOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    switch (key) {
    case MaaResOption_InferenceDevice:
        return set_inference_device(value, val_size);

    case MaaResOption_InferenceExecutionProvider:
        return set_inference_execution_provider(value, val_size);

    default:
        LogError << "Unknown key" << VAR(key) << VAR(value);
        return false;
    }
}

MaaResId ResourceMgr::post_bundle(const std::filesystem::path& path)
{
    LogInfo << VAR(path);

    if (!check_stop()) {
        return MaaInvalidId;
    }

    valid_ = false;
    hash_cache_.clear();

    if (!res_loader_) {
        LogError << "res_loader_ is nullptr";
        return MaaInvalidId;
    }

    return res_loader_->post(path);
}

MaaStatus ResourceMgr::status(MaaResId res_id) const
{
    if (!res_loader_) {
        LogError << "res_loader_ is nullptr";
        return MaaStatus_Invalid;
    }
    return res_loader_->status(res_id);
}

MaaStatus ResourceMgr::wait(MaaResId res_id) const
{
    if (!res_loader_) {
        LogError << "res_loader_ is nullptr";
        return false;
    }
    res_loader_->wait(res_id);
    return res_loader_->status(res_id);
}

bool ResourceMgr::valid() const
{
    return valid_;
}

// https://stackoverflow.com/questions/20511347/a-good-hash-function-for-a-vector
size_t vec_hash(const std::vector<size_t>& vec)
{
    size_t seed = vec.size();
    for (auto x : vec) {
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = (x >> 16) ^ x;
        seed ^= x + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
}

std::string ResourceMgr::get_hash() const
{
    return hash_cache_;
}

std::vector<std::string> ResourceMgr::get_node_list() const
{
    return pipeline_res_.get_node_list();
}

void ResourceMgr::post_stop()
{
    LogFunc;

    need_to_stop_ = true;

    if (res_loader_ && res_loader_->running()) {
        res_loader_->clear();
    }
}

std::string ResourceMgr::calc_hash()
{
    std::vector<size_t> filesizes;
    for (const auto& p : paths_) {
        if (!std::filesystem::exists(p) || !std::filesystem::is_directory(p)) {
            LogError << "path not exists or not a directory" << VAR(p);
            continue;
        }

        for (const auto& entry : std::filesystem::recursive_directory_iterator(p)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            filesizes.emplace_back(entry.file_size());
        }
    }
    size_t hash = vec_hash(filesizes);

    std::stringstream ss;
    ss << std::hex << hash;
    hash_cache_ = std::move(ss).str();

    LogInfo << VAR(hash_cache_);
    return hash_cache_;
}

bool ResourceMgr::running() const
{
    return res_loader_ && res_loader_->running();
}

bool ResourceMgr::clear()
{
    LogFunc;

    if (running()) {
        LogError << "running, ignore clear";
        return false;
    }

    pipeline_res_.clear();
    ocr_res_.clear();
    onnx_res_.clear();
    template_res_.clear();
    paths_.clear();
    hash_cache_.clear();

    valid_ = true;

    return true;
}

bool ResourceMgr::override_pipeline(const json::object& pipeline_override)
{
    LogFunc << VAR(pipeline_override);

    std::set<std::string> existing_keys;
    return pipeline_res_.parse_and_override(pipeline_override, existing_keys, default_pipeline_);
}

bool ResourceMgr::override_next(const std::string& node_name, const std::vector<std::string>& next)
{
    LogFunc << VAR(node_name) << VAR(next);
    pipeline_res_.get_pipeline_data_map()[node_name].next = next;
    return true;
}

void ResourceMgr::register_custom_recognition(const std::string& name, MaaCustomRecognitionCallback recognition, void* trans_arg)
{
    LogDebug << VAR(name) << VAR_VOIDP(recognition) << VAR_VOIDP(trans_arg);

    if (name.empty() || !recognition) {
        LogError << "empty name or handle";
        return;
    }
    custom_recognition_sessions_.insert_or_assign(name, CustomRecognitionSession { .recognition = recognition, .trans_arg = trans_arg });
}

void ResourceMgr::unregister_custom_recognition(const std::string& name)
{
    LogDebug << VAR(name);

    if (name.empty()) {
        LogError << "empty name or handle";
        return;
    }
    custom_recognition_sessions_.erase(name);
}

void ResourceMgr::clear_custom_recognition()
{
    LogTrace;

    custom_recognition_sessions_.clear();
}

void ResourceMgr::register_custom_action(const std::string& name, MaaCustomActionCallback action, void* trans_arg)
{
    LogDebug << VAR(name) << VAR_VOIDP(action) << VAR_VOIDP(trans_arg);

    if (name.empty() || !action) {
        LogError << "empty name or handle";
        return;
    }
    custom_action_sessions_.insert_or_assign(name, CustomActionSession { .action = action, .trans_arg = trans_arg });
}

void ResourceMgr::unregister_custom_action(const std::string& name)
{
    LogDebug << VAR(name);

    if (name.empty()) {
        LogError << "empty name or handle";
        return;
    }
    custom_action_sessions_.erase(name);
}

void ResourceMgr::clear_custom_action()
{
    LogTrace;

    custom_action_sessions_.clear();
}

CustomRecognitionSession ResourceMgr::custom_recognition(const std::string& name) const
{
    auto it = custom_recognition_sessions_.find(name);
    if (it == custom_recognition_sessions_.end()) {
        return {};
    }

    return it->second;
}

CustomActionSession ResourceMgr::custom_action(const std::string& name) const
{
    auto it = custom_action_sessions_.find(name);
    if (it == custom_action_sessions_.end()) {
        return {};
    }

    return it->second;
}

const std::unordered_set<MaaInferenceExecutionProvider>& ResourceMgr::available_providers()
{
    static std::unordered_set<MaaInferenceExecutionProvider> s_provider_cache;
    if (!s_provider_cache.empty()) {
        return s_provider_cache;
    }

    auto all_providers_vec = Ort::GetAvailableProviders();
    LogInfo << VAR(all_providers_vec);

    for (const auto& name : all_providers_vec) {
        if (name == "CPUExecutionProvider") {
            s_provider_cache.emplace(MaaInferenceExecutionProvider_CPU);
        }
        else if (name == "DmlExecutionProvider") {
#ifdef MAA_WITH_DML
            s_provider_cache.emplace(MaaInferenceExecutionProvider_DirectML);
#else
            LogDebug << "MaaFW built without DirectML";
#endif
        }
        else if (name == "CoreMLExecutionProvider") {
#ifdef MAA_WITH_COREML
            s_provider_cache.emplace(MaaInferenceExecutionProvider_CoreML);
#else
            LogDebug << "MaaFW built without CoreML";
#endif
        }
        else if (name == "CUDAExecutionProvider") {
            s_provider_cache.emplace(MaaInferenceExecutionProvider_CUDA);
        }
        else {
            LogDebug << "unsupported provider" << VAR(name);
        }
    }

    LogInfo << VAR(s_provider_cache);
    return s_provider_cache;
}

bool ResourceMgr::set_inference_device(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR_VOIDP(value) << VAR(val_size);

    if (val_size != sizeof(MaaInferenceDevice)) {
        LogError << "invalid size" << VAR(val_size);
        return false;
    }

    inference_device_setted_ = false;
    inference_device_ = *reinterpret_cast<MaaInferenceDevice*>(value);
    LogInfo << VAR(inference_device_);

    return true;
}

bool ResourceMgr::set_inference_execution_provider(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR_VOIDP(value) << VAR(val_size);

    if (val_size != sizeof(MaaInferenceExecutionProvider)) {
        LogError << "invalid size" << VAR(val_size);
        return false;
    }

    inference_device_setted_ = false;
    inference_ep_ = *reinterpret_cast<MaaInferenceExecutionProvider*>(value);
    LogInfo << VAR(inference_ep_);

    return true;
}

bool ResourceMgr::check_and_set_inference_device()
{
    if (inference_device_setted_) {
        return true;
    }

    if (inference_device_ == MaaInferenceDevice_CPU) {
        return use_cpu();
    }

    bool ret = false;
    switch (inference_ep_) {
    case MaaInferenceExecutionProvider_Auto:
        ret = use_auto_ep();
        break;
    case MaaInferenceExecutionProvider_CPU:
        ret = use_cpu();
        break;
    case MaaInferenceExecutionProvider_DirectML:
        ret = use_directml();
        break;
    case MaaInferenceExecutionProvider_CoreML:
        ret = use_coreml();
        break;
    default:
        LogError << "invalid inference execution provider" << VAR(inference_ep_);
        ret = false;
        break;
    }

    inference_device_setted_ = ret;

    if (!ret) {
        use_cpu();
    }
    return ret;
}

bool ResourceMgr::use_auto_ep()
{
    const auto& providers = available_providers();
    if (providers.contains(MaaInferenceExecutionProvider_CUDA)) {
        return use_cuda();
    }
    else if (providers.contains(MaaInferenceExecutionProvider_DirectML)) {
        return use_directml();
    }
    else if (providers.contains(MaaInferenceExecutionProvider_CoreML)) {
        return use_coreml();
    }
    else {
        return use_cpu();
    }
}

bool ResourceMgr::use_cpu()
{
    onnx_res_.use_cpu();
    ocr_res_.use_cpu();
    return true;
}

bool ResourceMgr::use_directml()
{
    const auto& providers = available_providers();
    if (!providers.contains(MaaInferenceExecutionProvider_DirectML)) {
        LogError << "DirectML is not available";
        return false;
    }

    int device_id = 0;
    if (inference_device_ == MaaInferenceDevice_CPU) {
        LogError << "Invalid device: MaaInferenceDevice_CPU for DirectML";
        return false;
    }
    else if (inference_device_ == MaaInferenceDevice_Auto) {
        auto gpu_id = perfer_gpu();
        if (!gpu_id) {
            LogError << "No suitable inference GPU for DirectML";
            return false;
        }
        device_id = *gpu_id;
    }
    else if (inference_device_ >= MaaInferenceDevice_0) {
        device_id = inference_device_;
    }
    else {
        LogError << "invalid inference device" << VAR(inference_device_);
        return false;
    }

    onnx_res_.use_directml(device_id);
    ocr_res_.use_directml(device_id);
    return true;
}

bool ResourceMgr::use_coreml()
{
    const auto& providers = available_providers();
    if (!providers.contains(MaaInferenceExecutionProvider_CoreML)) {
        LogError << "CoreML is not available";
        return false;
    }

    uint32_t coreml_flag = 0;
    if (inference_device_ == MaaInferenceDevice_CPU) {
        LogError << "Invalid device: MaaInferenceDevice_CPU for CoreML";
        return false;
    }
    else if (inference_device_ == MaaInferenceDevice_Auto) {
#ifdef MAA_WITH_COREML
        coreml_flag = COREMLFlags::COREML_FLAG_ONLY_ENABLE_DEVICE_WITH_ANE;
#endif
    }
    else if (inference_device_ >= MaaInferenceDevice_0) {
        coreml_flag = static_cast<uint32_t>(inference_device_);
    }
    else {
        LogError << "invalid inference device" << VAR(inference_device_);
        return false;
    }

    onnx_res_.use_coreml(coreml_flag);
    ocr_res_.use_coreml(coreml_flag);
    return true;
}

bool ResourceMgr::use_cuda()
{
    const auto& providers = available_providers();
    if (!providers.contains(MaaInferenceExecutionProvider_CUDA)) {
        LogError << "CUDA is not available";
        return false;
    }

    int device_id = 0;
    if (inference_device_ == MaaInferenceDevice_CPU) {
        LogError << "Invalid device: MaaInferenceDevice_CPU for CUDA";
        return false;
    }
    else if (inference_device_ == MaaInferenceDevice_Auto) {
        // TODO
        device_id = 0;
    }
    else if (inference_device_ >= MaaInferenceDevice_0) {
        device_id = inference_device_;
    }
    else {
        LogError << "invalid inference device" << VAR(inference_device_);
        return false;
    }

    onnx_res_.use_cuda(device_id);
    ocr_res_.use_cuda(device_id);
    return true;
}

bool ResourceMgr::run_load(typename AsyncRunner<std::filesystem::path>::Id id, std::filesystem::path path)
{
    LogFunc << VAR(id) << VAR(path);

    json::value cb_detail = {
        { "res_id", id },
        { "path", path_to_utf8_string(path) },
        { "hash", get_hash() },
    };

    notifier_.notify(MaaMsg_Resource_Loading_Starting, cb_detail);

    valid_ = load(path);

    cb_detail["hash"] = calc_hash();

    notifier_.notify(valid_ ? MaaMsg_Resource_Loading_Succeeded : MaaMsg_Resource_Loading_Failed, cb_detail);

    return valid_;
}

bool ResourceMgr::load(const std::filesystem::path& path)
{
    LogFunc << VAR(path);

    if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
        LogError << "path not exists or not a directory" << VAR(path);
        return false;
    }

    check_and_set_inference_device();

    paths_.emplace_back(path);

    using namespace path_literals;

    bool to_load = false;
    bool ret = true;
    if (auto p = path / "default_pipeline.json"_path; std::filesystem::exists(p)) {
        to_load = true;
        ret &= default_pipeline_.load(p);
    }
    if (auto p = path / "pipeline"_path; std::filesystem::exists(p)) {
        to_load = true;
        ret &= pipeline_res_.load(p, default_pipeline_);
    }
    if (auto p = path / "model"_path / "ocr"_path; std::filesystem::exists(p)) {
        to_load = true;
        ret &= ocr_res_.lazy_load(p);
    }
    if (auto p = path / "model"_path / "classify"_path; std::filesystem::exists(p)) {
        to_load = true;
        ret &= onnx_res_.lazy_load_classifier(p);
    }
    if (auto p = path / "model"_path / "detect"_path; std::filesystem::exists(p)) {
        to_load = true;
        ret &= onnx_res_.lazy_load_detector(p);
    }
    if (auto p = path / "image"_path; std::filesystem::exists(p)) {
        to_load = true;
        ret &= template_res_.lazy_load(p);
    }

    LogInfo << VAR(path) << VAR(ret) << VAR(to_load);

    return to_load && ret;
}

bool ResourceMgr::check_stop()
{
    if (!need_to_stop_) {
        return true;
    }

    if (running()) {
        LogError << "stopping, ignore new post";
        return false;
    }

    need_to_stop_ = false;
    return true;
}

MAA_RES_NS_END
