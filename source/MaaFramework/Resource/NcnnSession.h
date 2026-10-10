#pragma once

#ifdef MAA_WITH_NCNN

#include <filesystem>
#include <memory>
#include <mutex>

#include <allocator.h>
#include <net.h>

#include "Common/Conf.h"
#include "Vision/InferenceSession.h"

MAA_RES_NS_BEGIN

class NcnnSession : public MAA_VISION_NS::InferenceSession
{
public:
    // Model files are derived from base (the "model" field in pipeline, e.g. "AutoFight/xxx.onnx"):
    //   <stem>.ncnn.param / <stem>.ncnn.bin / <stem>.ncnn.json (sidecar, required)
    // returns nullptr on failure.
    static std::shared_ptr<NcnnSession> create(const std::filesystem::path& base, int num_threads);

    // all of .ncnn.param / .ncnn.bin / .ncnn.json exist
    static bool files_exist(const std::filesystem::path& base);
    // any of them exists
    static bool any_file_exists(const std::filesystem::path& base);

    // big + middle cores (exclude the lowest-frequency cluster), at most 4
    static int default_num_threads();

    // load_param + load_model, supports non-ascii paths
    static bool load_net_files(ncnn::Net& net, const std::filesystem::path& param, const std::filesystem::path& bin);

    virtual ~NcnnSession() override;

public:
    virtual MAA_VISION_NS::InferenceEngine engine() const override { return MAA_VISION_NS::InferenceEngine::NCNN; }

    virtual const std::vector<int64_t>& input_shape() const override { return input_shape_; }

    virtual std::optional<MAA_VISION_NS::InferenceOutput> run(const std::vector<float>& input) const override;

    virtual const std::unordered_map<std::string, std::string>& metadata() const override { return metadata_; }

private:
    NcnnSession() = default;

    bool load(const std::filesystem::path& param, const std::filesystem::path& bin, const std::filesystem::path& sidecar, int num_threads);
    bool load_sidecar(const std::filesystem::path& sidecar);

    // declaration order matters: allocators must outlive net_
    mutable ncnn::UnlockedPoolAllocator blob_allocator_;
    mutable ncnn::UnlockedPoolAllocator workspace_allocator_;
    ncnn::Net net_;
    // UnlockedPoolAllocator is not thread-safe
    mutable std::mutex run_mutex_;

    std::string input_name_;
    std::string output_name_;
    std::vector<int64_t> input_shape_;
    std::unordered_map<std::string, std::string> metadata_;
};

MAA_RES_NS_END

#endif // MAA_WITH_NCNN
