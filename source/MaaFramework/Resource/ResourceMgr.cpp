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

    loaded_ = false;
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

MaaBool ResourceMgr::loaded() const
{
    return loaded_;
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
        for (const auto& entry : std::filesystem::recursive_directory_iterator(p)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            filesizes.emplace_back(entry.file_size());
        }
    }
    size_t hash = vec_hash(filesizes);
    LogInfo << VAR(hash);

    hash_cache_ = std::to_string(hash);
    return hash_cache_;
}

std::vector<std::string> ResourceMgr::get_task_list() const
{
    return pipeline_res_.get_task_list();
}

bool ResourceMgr::run_load(typename AsyncRunner<std::filesystem::path>::Id id, std::filesystem::path path)
{
    LogFunc << VAR(id) << VAR(path);

    const json::value details = {
        { "id", id },
        { "path", path_to_utf8_string(path) },
    };

    notifier.notify(MaaMsg_Resource_StartLoading, details);

    loaded_ = load(path);

    notifier.notify(loaded_ ? MaaMsg_Resource_LoadingCompleted : MaaMsg_Resource_LoadingFailed, details);

    return loaded_;
}

bool ResourceMgr::load(const std::filesystem::path& path)
{
    LogFunc << VAR(path);

    using namespace path_literals;

    json::value props = json::open(path / "properties.json"_path).value_or(json::value());
    LogInfo << VAR(props);

    bool is_base = props.get("is_base", false);

    bool ret = pipeline_res_.load(path / "pipeline"_path, is_base);
    ret &= ocr_res_.lazy_load(path / "model"_path / "ocr"_path, is_base);
    ret &= onnx_res_.lazy_load(path / "model"_path, is_base);
    ret &= template_res_.lazy_load(path / "image"_path, is_base);

    LogInfo << VAR(path) << VAR(ret);

    return ret;
}

MAA_RES_NS_END
