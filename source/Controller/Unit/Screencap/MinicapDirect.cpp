#include "MinicapDirect.h"

#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"

#include <format>

MAA_CTRL_UNIT_NS_BEGIN

bool MinicapDirect::parse(const json::value& config)
{
    return binary_->parse(config) && library_->parse(config);
}

// x86_64的prebuilt里面的library是32位的, 用不了
// arm64-v8会卡住, 不知道原因
static const std::string_view archList[] = { /* "x86_64", */ "x86", /* "arm64-v8a", */ "armeabi-v7a", "armeabi" };
static const int sdkList[] = { 14, 15, 16, 17, 18, 19, 21, 22, 23, 24, 25, 26, 27, 28, 29, 31 };
constexpr size_t archCount = sizeof(archList) / sizeof(archList[0]);

bool MinicapDirect::init(int w, int h, std::function<std::string(const std::string&)> path_of_bin,
                         std::function<std::string(const std::string&, int)> path_of_lib, const std::string& force_temp)
{
    LogFunc;

    if (!io_ptr_) {
        LogError << "io_ptr is nullptr";
        return false;
    }

    if (!binary_->init(force_temp) || !library_->init("minicap.so")) {
        return false;
    }

    auto archs = binary_->abilist();
    auto sdk = binary_->sdk();

    if (!archs || !sdk) {
        return false;
    }

    std::string targetArch {};
    for (const auto& arch : archs.value()) {
        auto it = std::find(archList, archList + archCount, arch);
        if (it != archList + archCount) {
            targetArch = arch;
            break;
        }
    }
    if (targetArch.empty()) {
        return false;
    }

    int fit_sdk = -1;
    for (auto s : sdkList) {
        if (s <= sdk.value()) {
            fit_sdk = s;
        }
        else {
            break;
        }
    }
    if (fit_sdk == -1) {
        return false;
    }

    auto bin = path_of_bin(targetArch);
    auto lib = path_of_lib(targetArch, fit_sdk);

    if (!binary_->push(bin) || !library_->push(lib)) {
        return false;
    }

    if (!binary_->chmod() || !library_->chmod()) {
        return false;
    }

    set_wh(w, h);
    return true;
}

std::optional<cv::Mat> MinicapDirect::screencap()
{
    auto res = binary_->invoke_bin_stdout(std::format("-P {}x{}@{}x{}/{} -s", width_, height_, width_, height_, 0));

    if (!res) {
        return std::nullopt;
    }

    const auto& buffer = res.value();
    auto pos = buffer.find("\n\xff\xd8"); // FFD8是JPG文件头

    if (pos == std::string::npos) {
        return std::nullopt;
    }

    auto data = buffer.substr(pos + 1);

    return process_data(data, std::bind(&ScreencapBase::decode_jpg, this, std::placeholders::_1));
}
MAA_CTRL_UNIT_NS_END
