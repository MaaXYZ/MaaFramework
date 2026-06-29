#include "DeviceInfo.h"

#include "MaaUtils/Logger.h"
#include "MaaUtils/Uuid.h"

MAA_CTRL_UNIT_NS_BEGIN

bool DeviceInfo::parse(const json::value& config)
{
    static const json::array kDefaultUuidArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "shell", "settings get secure android_id",
    };
    static const json::array kDefaultResolutionArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "shell", "dumpsys window displays | grep DisplayFrames | tail -n 1 | grep -o -E [0-9]+",
    };

    // 兼容两类输出：
    // 1. 旧逻辑：SurfaceOrientation
    // 2. 部分 vivo / 新 Android 输出：Viewport INTERNAL ... orientation=0
    //
    // 使用 sed 而不是 grep 管道的原因：
    // grep 匹配不到时会返回非 0，导致 startup_and_read_pipe 判定 child return error。
    // sed 即使没有匹配到内容，一般也会正常退出；后续由 request_orientation 判断空输出。
    static const json::array kDefaultOrientationArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "shell",
        "dumpsys input | sed -n 's/.*SurfaceOrientation[^0-9]*\\([0-3]\\).*/\\1/p; s/.*Viewport INTERNAL.*orientation=\\([0-3]\\).*/\\1/p' | tail -n 1",
    };

    return parse_command("UUID", config, kDefaultUuidArgv, uuid_argv_)
           && parse_command("Resolution", config, kDefaultResolutionArgv, resolution_argv_)
           && parse_command("Orientation", config, kDefaultOrientationArgv, orientation_argv_);
}

std::optional<std::string> DeviceInfo::request_uuid()
{
    LogFunc;

    auto argv_opt = uuid_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return std::nullopt;
    }

    auto output_opt = startup_and_read_pipe(*argv_opt);
    if (!output_opt) {
        return make_uuid();
    }

    auto& uuid_str = output_opt.value();
    std::erase_if(uuid_str, [](unsigned char c) { return !std::isdigit(c) && !std::isalpha(c); });

    return uuid_str;
}

std::optional<DeviceInfo::DisplayInfo> DeviceInfo::request_resolution()
{
    LogFunc;

    auto argv_opt = resolution_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return std::nullopt;
    }

    auto output_opt = startup_and_read_pipe(*argv_opt);
    if (!output_opt) {
        return std::nullopt;
    }

    DisplayInfo info;

    std::istringstream iss(output_opt.value());
    iss >> info.w >> info.h >> info.r;

    return info;
}

std::optional<int> DeviceInfo::request_orientation()
{
    LogFunc;

    auto argv_opt = orientation_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return std::nullopt;
    }

    auto output_opt = startup_and_read_pipe(*argv_opt);
    if (!output_opt) {
        return std::nullopt;
    }

    const auto& s = output_opt.value();

    // 原逻辑只取 s.front()，如果输出前面带空格、换行、日志文本等，会误判。
    // 这里改成扫描第一个合法方向值。
    for (unsigned char c : s) {
        if (c >= '0' && c <= '3') {
            return static_cast<int>(c - '0');
        }
    }

    return std::nullopt;
}

MAA_CTRL_UNIT_NS_END
