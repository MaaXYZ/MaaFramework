#include "AdbShellInput.h"

#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

bool AdbShellInput::parse(const json::value& config)
{
    static const json::array kDefaultClickArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "shell", "input tap {X} {Y}",
    };
    static const json::array kDefaultSwipeArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "shell", "input swipe {X1} {Y1} {X2} {Y2} {DURATION}",
    };
    static const json::array kDefaultClickKeyArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "shell", "input keyevent {KEY}",
    };
    static const json::array kDefaultInputTextArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "shell", "input text '{TEXT}'",
    };

    return parse_command("Click", config, kDefaultClickArgv, click_argv_) && parse_command("Swipe", config, kDefaultSwipeArgv, swipe_argv_)
           && parse_command("ClickKey", config, kDefaultClickKeyArgv, click_key_argv_)
           && parse_command("InputText", config, kDefaultInputTextArgv, input_text_argv_);
}

MaaControllerFeature AdbShellInput::get_features() const
{
    return MaaControllerFeature_None;
}

bool AdbShellInput::click(int x, int y)
{
    LogInfo << VAR(x) << VAR(y);

    merge_replacement({ { "{X}", std::to_string(x) }, { "{Y}", std::to_string(y) } });

    auto argv_opt = click_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return false;
    }

    auto output_opt = startup_and_read_pipe(*argv_opt);
    return output_opt && output_opt->empty();
}

bool AdbShellInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogInfo << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);

    merge_replacement(
        { { "{X1}", std::to_string(x1) },
          { "{Y1}", std::to_string(y1) },
          { "{X2}", std::to_string(x2) },
          { "{Y2}", std::to_string(y2) },
          { "{DURATION}", duration ? std::to_string(duration) : std::string() } });

    auto argv_opt = swipe_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return false;
    }

    using namespace std::chrono_literals;
    auto output_opt = startup_and_read_pipe(*argv_opt);
    return output_opt && output_opt->empty();
}

bool AdbShellInput::touch_down(int contact, int x, int y, int pressure)
{
    LogError << "AdbShellInput not supports" << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);
    return false;
}

bool AdbShellInput::touch_move(int contact, int x, int y, int pressure)
{
    LogError << "AdbShellInput not supports" << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);
    return false;
}

bool AdbShellInput::touch_up(int contact)
{
    LogError << "AdbShellInput not supports" << VAR(contact);
    return false;
}

bool AdbShellInput::click_key(int key)
{
    LogInfo << VAR(key);

    merge_replacement({ { "{KEY}", std::to_string(key) } });

    auto argv_opt = click_key_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return false;
    }

    auto output_opt = startup_and_read_pipe(*argv_opt);
    return output_opt && output_opt->empty();
}

bool AdbShellInput::input_text(const std::string& text)
{
    LogInfo << VAR(text);

    merge_replacement({ { "{TEXT}", text } });

    auto argv_opt = input_text_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return false;
    }

    auto output_opt = startup_and_read_pipe(*argv_opt);
    return output_opt && output_opt->empty();
}

bool AdbShellInput::key_down(int key)
{
    LogError << "AdbShellInput not supports" << VAR(key);
    return false;
}

bool AdbShellInput::key_up(int key)
{
    LogError << "AdbShellInput not supports" << VAR(key);
    return false;
}

bool AdbShellInput::scroll(int dx, int dy)
{
    LogError << "Scroll is not supported on Adb controller" << VAR(dx) << VAR(dy);
    return false;
}

MAA_CTRL_UNIT_NS_END
