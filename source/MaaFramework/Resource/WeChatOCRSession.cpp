#include "WeChatOCRSession.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <limits>

#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "WeChatOCRABI.h"

#ifdef _WIN32
#include "MaaUtils/SafeWindows.hpp"
#elif defined(__linux__) && !defined(__ANDROID__)
#include <dlfcn.h>
#endif

MAA_RES_NS_BEGIN

namespace abi = wechat_ocr_abi;

struct WeChatOCRSession::Impl
{
#ifdef _WIN32
    HMODULE module = nullptr;
#else
    void* module = nullptr;
#endif
    abi::Engine* engine = nullptr;
    abi::LastError last_error = nullptr;
    abi::Create create = nullptr;
    abi::CreateAuto create_auto = nullptr;
    abi::Recognize recognize = nullptr;
    abi::GetCapabilities capabilities = nullptr;
    abi::LineCount line_count = nullptr;
    abi::GetInfo get_info = nullptr;
    abi::GetLine get_line = nullptr;
    abi::ResultDestroy result_destroy = nullptr;
    abi::Destroy destroy = nullptr;
    uint32_t request_timeout_ms = 10000;

    ~Impl()
    {
        if (engine && destroy) {
            destroy(engine);
        }
#ifdef _WIN32
        if (module) {
            FreeLibrary(module);
        }
#elif defined(__linux__) && !defined(__ANDROID__)
        if (module) {
            dlclose(module);
        }
#endif
    }
};

WeChatOCRSession::WeChatOCRSession(std::unique_ptr<Impl> impl)
    : impl_(std::move(impl))
{
}

WeChatOCRSession::~WeChatOCRSession() = default;

namespace
{
[[maybe_unused]] const int kModuleAnchor = 0;

std::filesystem::path framework_directory()
{
    std::filesystem::path module_path;
#ifdef _WIN32
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&kModuleAnchor),
            &module)) {
        return {};
    }
    std::wstring filename(32768, L'\0');
    DWORD length = GetModuleFileNameW(module, filename.data(), static_cast<DWORD>(filename.size()));
    if (length == 0 || length >= filename.size()) {
        return {};
    }
    filename.resize(length);
    module_path = std::filesystem::path(filename);
#elif defined(__linux__) && !defined(__ANDROID__)
    Dl_info info {};
    if (dladdr(&kModuleAnchor, &info) == 0 || !info.dli_fname) {
        return {};
    }
    module_path = MAA_NS::path(info.dli_fname);
#endif
    return module_path.is_absolute() ? module_path.parent_path() : std::filesystem::path {};
}

std::string default_sdk_path()
{
    const auto directory = framework_directory();
    if (directory.empty()) {
        LogError << "Could not determine MaaFramework module directory";
        return {};
    }
#ifdef _WIN32
    const auto filename = "WeChatOCR.dll";
#else
    const auto filename = "libWeChatOCR.so";
#endif
    for (const auto& candidate : { directory / filename, directory.parent_path() / "lib" / filename }) {
        std::error_code ec;
        if (std::filesystem::is_regular_file(candidate, ec)) {
            return MAA_NS::path_to_utf8_string(candidate);
        }
    }
    LogError << "WeChatOCR SDK not found beside MaaFramework or in ../lib" << VAR(directory);
    return {};
}

bool absolute_path(const json::value& config, const char* key, std::string& output, bool required)
{
    auto value = config.find<std::string>(key);
    if (!value) {
        return !required && !config.contains(key);
    }
    output = *value;
    if (output.empty() || output.find('\0') != std::string::npos || !MAA_NS::path(output).is_absolute()) {
        LogError << "WeChatOCR needs an absolute UTF-8 path" << VAR(key);
        return false;
    }
    return true;
}

bool uint_option(const json::value& config, const char* key, uint32_t& output)
{
    if (!config.contains(key)) {
        return true;
    }
    auto value = config.find<int64_t>(key);
    if (!value || *value <= 0 || *value > std::numeric_limits<uint32_t>::max()) {
        LogError << "Invalid WeChatOCR option" << VAR(key);
        return false;
    }
    output = static_cast<uint32_t>(*value);
    return true;
}

template <typename Module, typename T>
bool resolve(Module module, const char* name, T& function)
{
#ifdef _WIN32
    function = reinterpret_cast<T>(GetProcAddress(module, name));
#elif defined(__linux__) && !defined(__ANDROID__)
    function = reinterpret_cast<T>(dlsym(module, name));
#else
    (void)module;
    function = nullptr;
#endif
    if (!function) {
        LogError << "Missing WeChatOCR SDK symbol" << VAR(name);
        return false;
    }
    return true;
}
}

std::shared_ptr<WeChatOCRSession> WeChatOCRSession::create(const json::value& config)
{
#if !(defined(_WIN64) && !defined(_M_ARM64) && !defined(__aarch64__)) \
    && !(defined(__linux__) && defined(__x86_64__) && !defined(__ANDROID__))
    (void)config;
    LogError << "WeChatOCR is only supported on Windows x64 and Linux x64";
    return nullptr;
#endif
    if (!config.is_object()) {
        return nullptr;
    }
    for (const auto& [key, value] : config.as_object()) {
        if (key != "sdk_path" && key != "runtime_dir" && key != "ocr_path" && key != "worker_path" && key != "temp_dir" && key != "protocol"
            && key != "startup_timeout_ms" && key != "request_timeout_ms" && key != "max_pending_requests" && key != "search_roots"
            && key != "scan_system") {
            LogError << "Unknown WeChatOCR option" << VAR(key);
            return nullptr;
        }
    }
    std::string sdk_path, runtime_dir, ocr_path, worker_path, temp_dir;
    if (!absolute_path(config, "sdk_path", sdk_path, false) || !absolute_path(config, "runtime_dir", runtime_dir, false)
        || !absolute_path(config, "ocr_path", ocr_path, false) || !absolute_path(config, "worker_path", worker_path, false)
        || !absolute_path(config, "temp_dir", temp_dir, false)) {
        return nullptr;
    }
    if (runtime_dir.empty() != ocr_path.empty()) {
        LogError << "Provide both runtime_dir and ocr_path, or omit both for automatic discovery";
        return nullptr;
    }
    bool scan_system = true;
    if (config.contains("scan_system")) {
        auto value = config.find<bool>("scan_system");
        if (!value) {
            return nullptr;
        }
        scan_system = *value;
    }
    std::vector<std::string> search_roots;
    if (config.contains("search_roots")) {
        auto value = config.find<std::vector<std::string>>("search_roots");
        if (!value || value->size() > 64) {
            return nullptr;
        }
        search_roots = std::move(*value);
    }
    std::vector<const char*> root_pointers;
    for (const auto& root : search_roots) {
        if (root.empty() || root.find('\0') != std::string::npos || !MAA_NS::path(root).is_absolute()) {
            LogError << "WeChatOCR search roots must be absolute UTF-8 paths";
            return nullptr;
        }
        root_pointers.emplace_back(root.c_str());
    }
    if (sdk_path.empty()) {
        sdk_path = default_sdk_path();
        if (sdk_path.empty()) {
            return nullptr;
        }
    }
    uint32_t protocol = 0;
    if (config.contains("protocol")) {
        auto value = config.find<int64_t>("protocol");
        if (!value || (*value != 0 && *value != 3 && *value != 4)) {
            return nullptr;
        }
        protocol = static_cast<uint32_t>(*value);
    }
    auto impl = std::make_unique<Impl>();
    uint32_t startup_timeout_ms = 10000, max_pending_requests = 16;
    if (!uint_option(config, "startup_timeout_ms", startup_timeout_ms)
        || !uint_option(config, "request_timeout_ms", impl->request_timeout_ms)
        || !uint_option(config, "max_pending_requests", max_pending_requests)) {
        return nullptr;
    }
#ifdef _WIN32
    impl->module =
        LoadLibraryExW(MAA_NS::path(sdk_path).c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!impl->module) {
        LogError << "Failed to load WeChatOCR SDK" << VAR(sdk_path) << VAR(GetLastError());
        return nullptr;
    }
#elif defined(__linux__) && !defined(__ANDROID__)
    impl->module = dlopen(sdk_path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!impl->module) {
        LogError << "Failed to load WeChatOCR SDK" << VAR(sdk_path) << VAR(dlerror());
        return nullptr;
    }
#endif
    abi::Version version = nullptr;
    if (!resolve(impl->module, "wco_version", version) || !resolve(impl->module, "wco_last_error", impl->last_error)
        || !resolve(impl->module, "wco_create", impl->create) || !resolve(impl->module, "wco_recognize", impl->recognize)
        || !resolve(impl->module, "wco_get_capabilities", impl->capabilities)
        || !resolve(impl->module, "wco_result_line_count", impl->line_count)
        || !resolve(impl->module, "wco_result_get_info", impl->get_info) || !resolve(impl->module, "wco_result_get_line", impl->get_line)
        || !resolve(impl->module, "wco_result_destroy", impl->result_destroy) || !resolve(impl->module, "wco_destroy", impl->destroy)) {
        return nullptr;
    }
    const char* sdk_version = version();
    if (!sdk_version || !*sdk_version) {
        LogError << "WeChatOCR SDK has no version";
        return nullptr;
    }
    const bool use_auto = runtime_dir.empty() || !search_roots.empty();
    if (use_auto && !resolve(impl->module, "wco_create_auto", impl->create_auto)) {
        LogError << "WeChatOCR auto discovery requires SDK 0.2.0 or newer";
        return nullptr;
    }
    abi::Options options {
        .struct_size = sizeof(abi::Options),
        .abi_version = abi::kVersion,
        .runtime_dir = runtime_dir.empty() ? nullptr : runtime_dir.c_str(),
        .ocr_path = ocr_path.empty() ? nullptr : ocr_path.c_str(),
        .worker_path = worker_path.empty() ? nullptr : worker_path.c_str(),
        .temp_dir = temp_dir.empty() ? nullptr : temp_dir.c_str(),
        .protocol = protocol,
        .startup_timeout_ms = startup_timeout_ms,
        .max_pending_requests = max_pending_requests,
    };
    abi::DiscoveryOptions discovery {
        .struct_size = sizeof(abi::DiscoveryOptions),
        .flags = scan_system ? 1u : 0u,
        .search_roots = root_pointers.empty() ? nullptr : root_pointers.data(),
        .search_root_count = static_cast<uint32_t>(root_pointers.size()),
    };
    auto status = use_auto ? impl->create_auto(&options, &discovery, &impl->engine) : impl->create(&options, &impl->engine);
    if (status != 0 || !impl->engine) {
        LogError << "WeChatOCR SDK creation failed" << VAR(status) << VAR(impl->last_error());
        return nullptr;
    }
    abi::Capabilities capabilities { .struct_size = sizeof(abi::Capabilities) };
    if (impl->capabilities(impl->engine, &capabilities) != 0 || capabilities.struct_size < sizeof(abi::Capabilities)
        || capabilities.abi_version != abi::kVersion || !capabilities.detect_and_recognize) {
        LogError << "Incompatible WeChatOCR SDK ABI or capabilities" << VAR(sdk_version);
        return nullptr;
    }
    LogInfo << "WeChatOCR SDK ready" << VAR(sdk_version);
    return std::shared_ptr<WeChatOCRSession>(new WeChatOCRSession(std::move(impl)));
}

std::optional<MAA_VISION_NS::OCRDetRecResult> WeChatOCRSession::det_and_rec(const cv::Mat& image)
{
    if (image.empty() || image.type() != CV_8UC3 || !impl_ || !impl_->engine) {
        return std::nullopt;
    }
    abi::Image view {
        .struct_size = sizeof(abi::Image),
        .format = 1,
        .data = image.data,
        .data_size = image.step[0] * (image.rows - 1) + image.cols * image.elemSize(),
        .width = static_cast<uint32_t>(image.cols),
        .height = static_cast<uint32_t>(image.rows),
        .stride = image.step[0],
    };
    abi::Result* raw_result = nullptr;
    auto status = impl_->recognize(impl_->engine, &view, impl_->request_timeout_ms, &raw_result);
    std::unique_ptr<abi::Result, abi::ResultDestroy> result(raw_result, impl_->result_destroy);
    if (status != 0 || !result) {
        LogError << "WeChatOCR recognition failed" << VAR(status) << VAR(impl_->last_error());
        return std::nullopt;
    }
    abi::ResultInfo info { .struct_size = sizeof(abi::ResultInfo) };
    if (impl_->get_info(result.get(), &info) != 0 || info.struct_size < sizeof(abi::ResultInfo) || info.vendor_error != 0
        || info.width != view.width || info.height != view.height) {
        LogError << "Invalid WeChatOCR result metadata" << VAR(info.vendor_error) << VAR(info.width) << VAR(info.height);
        return std::nullopt;
    }
    MAA_VISION_NS::OCRDetRecResult output;
    uint32_t count = impl_->line_count(result.get());
    for (uint32_t index = 0; index < count; ++index) {
        abi::Line line { .struct_size = sizeof(abi::Line) };
        if (impl_->get_line(result.get(), index, &line) != 0 || line.struct_size < sizeof(abi::Line) || !line.text
            || line.text_length > std::numeric_limits<size_t>::max()) {
            LogError << "Invalid WeChatOCR line" << VAR(index) << VAR(impl_->last_error());
            return std::nullopt;
        }
        std::array<int, 8> box;
        for (size_t point = 0; point < box.size(); ++point) {
            const double coordinate = line.quad[point];
            const double limit = point % 2 == 0 ? image.cols : image.rows;
            // Runtime transforms can put a boundary point a fraction of a pixel outside the image.
            if (!std::isfinite(coordinate) || coordinate < -1.0 || coordinate > limit + 1.0) {
                LogError << "Invalid WeChatOCR coordinates" << VAR(index);
                return std::nullopt;
            }
            box[point] = static_cast<int>(std::lround(std::clamp(coordinate, 0.0, limit)));
        }
        output.boxes.emplace_back(box);
        output.texts.emplace_back(line.text, static_cast<size_t>(line.text_length));
        output.scores.emplace_back(line.has_confidence && std::isfinite(line.confidence) && line.confidence >= 0 ? line.confidence : -1.0f);
    }
    return output;
}

std::optional<MAA_VISION_NS::OCRRecResult> WeChatOCRSession::rec(const cv::Mat& image)
{
    (void)image;
    LogError << "WeChatOCR does not support only_rec";
    return std::nullopt;
}

std::optional<std::vector<MAA_VISION_NS::OCRRecResult>> WeChatOCRSession::batch_rec(const std::vector<cv::Mat>& images)
{
    (void)images;
    LogError << "WeChatOCR does not support batch_rec";
    return std::nullopt;
}

MAA_RES_NS_END
