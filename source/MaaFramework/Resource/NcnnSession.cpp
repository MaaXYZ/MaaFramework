#ifdef MAA_WITH_NCNN

#include "NcnnSession.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <map>
#include <unordered_map>

#include <cpu.h>

#include "MaaUtils/JsonExt.hpp"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"

MAA_RES_NS_BEGIN

namespace
{

std::filesystem::path derive_path(const std::filesystem::path& base, const char* ext)
{
    auto path = base;
    path.replace_extension(ext);
    return path;
}

struct FileCloser
{
    void operator()(FILE* fp) const
    {
        if (fp) {
            fclose(fp);
        }
    }
};

using FilePtr = std::unique_ptr<FILE, FileCloser>;

FilePtr open_file(const std::filesystem::path& path)
{
#ifdef _WIN32
    FILE* fp = nullptr;
    if (_wfopen_s(&fp, path.c_str(), L"rb") != 0) {
        return nullptr;
    }
    return FilePtr(fp);
#else
    return FilePtr(fopen(path.c_str(), "rb"));
#endif
}

} // namespace

bool NcnnSession::files_exist(const std::filesystem::path& base)
{
    return std::filesystem::exists(derive_path(base, ".ncnn.param")) && std::filesystem::exists(derive_path(base, ".ncnn.bin"))
           && std::filesystem::exists(derive_path(base, ".ncnn.json"));
}

bool NcnnSession::any_file_exists(const std::filesystem::path& base)
{
    return std::filesystem::exists(derive_path(base, ".ncnn.param")) || std::filesystem::exists(derive_path(base, ".ncnn.bin"))
           || std::filesystem::exists(derive_path(base, ".ncnn.json"));
}

int NcnnSession::default_num_threads()
{
    constexpr int kMaxThreads = 4;

    const int cpu_count = ncnn::get_cpu_count();
    int count = 0;

#ifndef _WIN32
    // ncnn::get_big_cpu_count() only counts the highest-frequency cluster,
    // which is 1 on 1+3+4 SoCs (e.g. SM8250). Count all cores faster than the slowest cluster instead.
    std::vector<long> freqs;
    for (int i = 0; i < cpu_count; ++i) {
        std::ifstream ifs(std::format("/sys/devices/system/cpu/cpu{}/cpufreq/cpuinfo_max_freq", i));
        long freq = 0;
        if (ifs >> freq) {
            freqs.emplace_back(freq);
        }
    }
    if (static_cast<int>(freqs.size()) == cpu_count && !freqs.empty()) {
        const long min_freq = *std::ranges::min_element(freqs);
        count = static_cast<int>(std::ranges::count_if(freqs, [&](long f) { return f > min_freq; }));
    }
#endif

    if (count <= 0) {
        // all cores have the same frequency, or frequency is unavailable
        count = cpu_count;
    }

    return std::clamp(count, 1, kMaxThreads);
}

ncnn::Option NcnnSession::make_option(int num_threads)
{
    ncnn::Option opt;
    opt.num_threads = num_threads > 0 ? num_threads : default_num_threads();
    opt.use_vulkan_compute = false;
    // fp32 by default, scores are compared with thresholds tuned on onnxruntime. a model opts in via its sidecar
    opt.use_fp16_packed = false;
    opt.use_fp16_storage = false;
    opt.use_fp16_arithmetic = false;
    opt.use_bf16_packed = false;
    opt.use_bf16_storage = false;
    return opt;
}

bool NcnnSession::apply_options(const json::value& sidecar, ncnn::Option& opt)
{
    if (!sidecar.contains("options")) {
        return true;
    }
    const auto& options = sidecar.at("options");
    if (!options.is_object()) {
        LogError << "ncnn options must be an object" << VAR(options);
        return false;
    }

    // only the precision a model tolerates is configurable, with the ncnn::Option field names.
    // device related options (threads, kernels, layout, ...) are handled by MaaFW / ncnn defaults
    static const std::unordered_map<std::string, bool ncnn::Option::*> kOptions = {
        { "use_fp16_packed", &ncnn::Option::use_fp16_packed },
        { "use_fp16_storage", &ncnn::Option::use_fp16_storage },
        { "use_fp16_arithmetic", &ncnn::Option::use_fp16_arithmetic },
    };

    for (const auto& [key, value] : options.as_object()) {
        auto iter = kOptions.find(key);
        if (iter == kOptions.end()) {
            LogWarn << "unknown ncnn option, ignored" << VAR(key);
            continue;
        }
        if (!value.is_boolean()) {
            LogError << "ncnn option expects a boolean" << VAR(key) << VAR(value);
            return false;
        }
        opt.*(iter->second) = value.as_boolean();
    }

    return true;
}

bool NcnnSession::apply_options_file(const std::filesystem::path& sidecar, ncnn::Option& opt)
{
    if (!std::filesystem::exists(sidecar)) {
        return true;
    }

    auto json_opt = json::open(sidecar);
    if (!json_opt || !json_opt->is_object()) {
        LogError << "failed to parse ncnn sidecar" << VAR(sidecar);
        return false;
    }
    return apply_options(*json_opt, opt);
}

bool NcnnSession::load_net_files(ncnn::Net& net, const std::filesystem::path& param, const std::filesystem::path& bin)
{
    auto param_fp = open_file(param);
    if (!param_fp || net.load_param(param_fp.get()) != 0) {
        LogError << "ncnn load_param failed" << VAR(param);
        return false;
    }

    auto bin_fp = open_file(bin);
    if (!bin_fp || net.load_model(bin_fp.get()) != 0) {
        LogError << "ncnn load_model failed" << VAR(bin);
        return false;
    }
    return true;
}

std::shared_ptr<NcnnSession> NcnnSession::create(const std::filesystem::path& base, int num_threads)
{
    LogFunc << VAR(base) << VAR(num_threads);

    auto param = derive_path(base, ".ncnn.param");
    auto bin = derive_path(base, ".ncnn.bin");
    auto sidecar = derive_path(base, ".ncnn.json");

    auto session = std::shared_ptr<NcnnSession>(new NcnnSession);
    if (!session->load(param, bin, sidecar, num_threads)) {
        return nullptr;
    }
    return session;
}

NcnnSession::~NcnnSession()
{
    net_.clear();
    blob_allocator_.clear();
    workspace_allocator_.clear();
}

bool NcnnSession::load(
    const std::filesystem::path& param,
    const std::filesystem::path& bin,
    const std::filesystem::path& sidecar,
    int num_threads)
{
    // options must be set before load_param / load_model
    ncnn::Option opt = make_option(num_threads);
    if (!load_sidecar(sidecar, opt)) {
        return false;
    }
    // owned by this session, not configurable
    opt.blob_allocator = &blob_allocator_;
    opt.workspace_allocator = &workspace_allocator_;
    net_.opt = opt;

    if (!load_net_files(net_, param, bin)) {
        return false;
    }

    if (input_name_.empty()) {
        if (net_.input_names().empty()) {
            LogError << "ncnn model has no input" << VAR(param);
            return false;
        }
        input_name_ = net_.input_names().front();
    }
    if (output_name_.empty()) {
        if (net_.output_names().empty()) {
            LogError << "ncnn model has no output" << VAR(param);
            return false;
        }
        output_name_ = net_.output_names().front();
    }

    LogInfo << "ncnn session loaded" << VAR(param) << VAR(input_name_) << VAR(output_name_) << VAR(input_shape_)
            << VAR(net_.opt.num_threads) << VAR(net_.opt.use_fp16_storage) << VAR(net_.opt.use_fp16_arithmetic);
    return true;
}

bool NcnnSession::load_sidecar(const std::filesystem::path& sidecar, ncnn::Option& opt)
{
    auto json_opt = json::open(sidecar);
    if (!json_opt || !json_opt->is_object()) {
        LogError << "failed to parse ncnn sidecar" << VAR(sidecar);
        return false;
    }
    const auto& root = *json_opt;

    input_name_ = root.get("input", std::string());
    output_name_ = root.get("output", std::string());

    // fixed NCHW, batch 1 only; C / H / W are used as int by ncnn and the vision preprocessing
    auto shape_opt = root.find<std::vector<int64_t>>("input_shape");
    if (!shape_opt || shape_opt->size() != 4 || shape_opt->front() != 1
        || std::ranges::any_of(*shape_opt, [](int64_t d) { return d <= 0 || d > std::numeric_limits<int>::max(); })) {
        LogError << "invalid input_shape in ncnn sidecar, expect [1, C, H, W]" << VAR(sidecar);
        return false;
    }
    input_shape_ = std::move(*shape_opt);

    if (auto meta_opt = root.find<json::object>("metadata")) {
        for (const auto& [key, value] : *meta_opt) {
            if (value.is_string()) {
                metadata_.emplace(key, value.as_string());
            }
        }
    }

    if (!apply_options(root, opt)) {
        LogError << "invalid options in ncnn sidecar" << VAR(sidecar);
        return false;
    }

    return true;
}

std::optional<MAA_VISION_NS::InferenceOutput> NcnnSession::run(const std::vector<float>& input) const
{
    const int c = static_cast<int>(input_shape_[1]);
    const int h = static_cast<int>(input_shape_[2]);
    const int w = static_cast<int>(input_shape_[3]);
    const size_t plane = static_cast<size_t>(w) * h;

    if (input_shape_[0] != 1 || input.size() != plane * c) {
        LogError << "invalid input size" << VAR(input_shape_) << VAR(input.size());
        return std::nullopt;
    }

    std::unique_lock lock(run_mutex_);

    // channels of a 3-dim ncnn::Mat are aligned to 16 bytes (cstep), copy channel by channel
    ncnn::Mat in(w, h, c, 4u, &blob_allocator_);
    if (in.empty()) {
        LogError << "failed to allocate ncnn input" << VAR(input_shape_);
        return std::nullopt;
    }
    for (int q = 0; q < c; ++q) {
        std::memcpy(in.channel(q), input.data() + plane * q, sizeof(float) * plane);
    }

    ncnn::Extractor ex = net_.create_extractor();
    if (ex.input(input_name_.c_str(), in) != 0) {
        LogError << "ncnn extractor input failed" << VAR(input_name_);
        return std::nullopt;
    }

    ncnn::Mat out;
    if (ex.extract(output_name_.c_str(), out) != 0 || out.empty()) {
        LogError << "ncnn extractor extract failed" << VAR(output_name_);
        return std::nullopt;
    }
    if (!is_unpacked_fp32(out)) {
        LogError << "unexpected ncnn output layout, expect unpacked fp32" << VAR(out.elemsize) << VAR(out.elempack);
        return std::nullopt;
    }

    // normalize to onnxruntime layout: only prepend the batch dim
    MAA_VISION_NS::InferenceOutput output;
    switch (out.dims) {
    case 1:
        output.shape = { 1, out.w };
        output.data.assign(static_cast<const float*>(out.data), static_cast<const float*>(out.data) + out.w);
        break;
    case 2:
        output.shape = { 1, out.h, out.w };
        output.data.assign(static_cast<const float*>(out.data), static_cast<const float*>(out.data) + static_cast<size_t>(out.w) * out.h);
        break;
    case 3: {
        output.shape = { 1, out.c, out.h, out.w };
        const size_t out_plane = static_cast<size_t>(out.w) * out.h;
        output.data.reserve(out_plane * out.c);
        for (int q = 0; q < out.c; ++q) {
            const float* ptr = out.channel(q);
            output.data.insert(output.data.end(), ptr, ptr + out_plane);
        }
        break;
    }
    default:
        LogError << "unsupported ncnn output dims" << VAR(out.dims);
        return std::nullopt;
    }

    return output;
}

MAA_RES_NS_END

#endif // MAA_WITH_NCNN
