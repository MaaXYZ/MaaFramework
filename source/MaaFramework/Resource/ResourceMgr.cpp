#include "ResourceMgr.h"

#include <tuple>

#include "MaaFramework/MaaMsg.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

MAA_RES_NS_BEGIN

ResourceMgr::ResourceMgr(MaaResourceCallback callback, MaaCallbackTransparentArg callback_arg)
    : notifier(callback, callback_arg)
{
    LogFunc << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

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
    std::ignore = key;
    std::ignore = value;
    std::ignore = val_size;

    return false;
}

MaaResId ResourceMgr::post_path(std::filesystem::path path)
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

    return res_loader_->post(std::move(path));
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

    valid_ = true;

    return true;
}

bool ResourceMgr::run_load(
    typename AsyncRunner<std::filesystem::path>::Id id,
    std::filesystem::path path)
{
    LogFunc << VAR(id) << VAR(path);

    json::value details = {
        { "id", id },
        { "path", path_to_utf8_string(path) },
    };

    notifier.notify(MaaMsg_Resource_StartLoading, details);

    valid_ = load(path);

    details.emplace("hash", get_hash());
    notifier.notify(
        valid_ ? MaaMsg_Resource_LoadingCompleted : MaaMsg_Resource_LoadingFailed,
        details);

    return valid_;
}

bool ResourceMgr::load(const std::filesystem::path& path)
{
    LogFunc << VAR(path);

    using namespace path_literals;

    json::value props = json::open(path / "properties.json"_path).value_or(json::value());
    LogInfo << VAR(props);

    bool is_base = props.get("is_base", false);
    if (is_base) {
        paths_.clear();
    }
    paths_.emplace_back(path);

    bool ret = pipeline_res_.load(path / "pipeline"_path, is_base);
    ret &= ocr_res_.lazy_load(path / "model"_path / "ocr"_path, is_base);
    ret &= onnx_res_.lazy_load(path / "model"_path, is_base);
    ret &= template_res_.lazy_load(path / "image"_path, is_base);

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