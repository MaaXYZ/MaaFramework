#include "ResourceMgr.h"

#include <tuple>

#include "MaaFramework/MaaMsg.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

MAA_RES_NS_BEGIN

ResourceMgr::ResourceMgr(MaaNotificationCallback notify, void* notify_trans_arg)
    : notifier(notify, notify_trans_arg)
{
    LogFunc << VAR_VOIDP(notify) << VAR_VOIDP(notify_trans_arg);

    res_loader_ = std::make_unique<AsyncRunner<std::filesystem::path>>(
        std::bind(&ResourceMgr::run_load, this, std::placeholders::_1, std::placeholders::_2));
}

ResourceMgr::~ResourceMgr()
{
    LogFunc;

    if (res_loader_) {
        res_loader_->release();
    }
}

bool ResourceMgr::set_option(MaaResOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    switch (key) {
    case MaaResOption_GpuId:
        return set_gpu_id(value, val_size);

    default:
        LogError << "Unknown key" << VAR(key) << VAR(value);
        return false;
    }
}

MaaResId ResourceMgr::post_path(const std::filesystem::path& path)
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

MaaBool ResourceMgr::valid() const
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
    if (!hash_cache_.empty()) {
        return hash_cache_;
    }

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

std::vector<std::string> ResourceMgr::get_task_list() const
{
    return pipeline_res_.get_task_list();
}

void ResourceMgr::post_stop()
{
    LogFunc;

    need_to_stop_ = true;

    if (res_loader_ && res_loader_->running()) {
        res_loader_->clear();
    }
}

MaaBool ResourceMgr::running() const
{
    return res_loader_ && res_loader_->running();
}

MaaBool ResourceMgr::clear()
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
    clear_custom_recognition();
    clear_custom_action();

    valid_ = true;

    return true;
}

void ResourceMgr::register_custom_recognition(const std::string& name, MaaCustomRecognitionCallback recognition, void* trans_arg)
{
    LogTrace << VAR(name) << VAR_VOIDP(recognition) << VAR_VOIDP(trans_arg);

    if (name.empty() || !recognition) {
        LogError << "empty name or handle";
        return;
    }
    custom_recognition_sessions_.insert_or_assign(name, CustomRecognitionSession { .recognition = recognition, .trans_arg = trans_arg });
}

void ResourceMgr::unregister_custom_recognition(const std::string& name)
{
    LogTrace << VAR(name);

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
    LogTrace << VAR(name) << VAR_VOIDP(action) << VAR_VOIDP(trans_arg);

    if (name.empty() || !action) {
        LogError << "empty name or handle";
        return;
    }
    custom_action_sessions_.insert_or_assign(name, CustomActionSession { .action = action, .trans_arg = trans_arg });
}

void ResourceMgr::unregister_custom_action(const std::string& name)
{
    LogTrace << VAR(name);

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

bool ResourceMgr::set_gpu_id(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR_VOIDP(value) << VAR(val_size);

    if (val_size != sizeof(int32_t)) {
        LogError << "invalid size" << VAR(val_size);
        return false;
    }

    int32_t gpu_id = *reinterpret_cast<int*>(value);
    LogInfo << VAR(gpu_id);

    if (gpu_id == INT32_MAX) {
        onnx_res_.use_cpu();
        ocr_res_.use_cpu();
    }
    else {
        onnx_res_.use_gpu(gpu_id);
        ocr_res_.use_gpu(gpu_id);
    }

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

    notifier.notify(MaaMsg_Resource_Loading_Starting, cb_detail);

    valid_ = load(path);
    hash_cache_.clear();

    cb_detail["hash"] = get_hash();

    notifier.notify(valid_ ? MaaMsg_Resource_Loading_Succeeded : MaaMsg_Resource_Loading_Failed, cb_detail);

    return valid_;
}

bool ResourceMgr::load(const std::filesystem::path& path)
{
    LogFunc << VAR(path);

    using namespace path_literals;

    paths_.emplace_back(path);

    bool ret = default_pipeline_.load(path / "default_pipeline.json"_path);
    ret &= pipeline_res_.load(path / "pipeline"_path, false, default_pipeline_);
    ret &= ocr_res_.lazy_load(path / "model"_path / "ocr"_path, false);
    ret &= onnx_res_.lazy_load(path / "model"_path, false);
    ret &= template_res_.lazy_load(path / "image"_path, false);

    LogInfo << VAR(path) << VAR(ret);

    return ret;
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
