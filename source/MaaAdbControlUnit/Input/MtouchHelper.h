#pragma once

#include "Base/UnitBase.h"

#include <filesystem>

#include "AdbShellInput.h"
#include "General/DeviceInfo.h"
#include "Invoke/InvokeApp.h"
#include "MaaUtils/IOStream/ChildPipeIOStream.h"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class MtouchHelper : public InputBase
{
public:
    MtouchHelper() { children_.emplace_back(device_info_); }

    virtual ~MtouchHelper() override = default;

public: // from InputBase
    virtual MaaControllerFeature get_features() const override;

    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

    virtual bool parse(const json::value& config) override;

    virtual bool click_key(int key) override;
    virtual bool input_text(const std::string& text) override;

    virtual bool key_down(int key) override;
    virtual bool key_up(int key) override;

protected:
    bool read_info();

    virtual std::pair<int, int> screen_to_touch(int x, int y);
    virtual std::pair<int, int> screen_to_touch(double x, double y);

    virtual std::string type_name() const;

    // minitouch common operations
    bool parse_minitouch_config(const json::value& config);
    bool push_minitouch();
    void remove_binary();

    // https://github.com/openstf/minitouch#writable-to-the-socket
    static constexpr std::string_view kDownFormat = "d {} {} {} {}\nc\n";
    static constexpr std::string_view kMoveFormat = "m {} {} {} {}\nc\n";
    static constexpr std::string_view kUpFormat = "u {}\nc\n";

    std::shared_ptr<ChildPipeIOStream> pipe_ios_ = nullptr;

    int display_width_ = 0;
    int display_height_ = 0;
    int touch_width_ = 0;
    int touch_height_ = 0;
    double xscale_ = 0;
    double yscale_ = 0;
    int press_ = 0;
    int orientation_ = 0;

    std::filesystem::path agent_path_;
    std::vector<std::string> arch_list_;
    std::shared_ptr<InvokeApp> invoke_app_ = nullptr;
    std::shared_ptr<AdbShellInput> adb_shell_input_ = nullptr;

protected:
    virtual bool request_display_info();

private:
    template <typename T1, typename T2>
    inline std::pair<int, int> screen_to_touch_impl(T1 x, T2 y)
    {
        auto make_pair = [](double x, double y) {
            return std::make_pair(static_cast<int>(round(x)), static_cast<int>(round(y)));
        };
        switch (orientation_) {
        case 0:
            return make_pair(x * xscale_, y * yscale_);
        case 1:
            return make_pair(touch_height_ - y * yscale_, x * xscale_);
        case 2:
            return make_pair(touch_width_ - x * xscale_, touch_height_ - y * yscale_);
        case 3:
            return make_pair(y * yscale_, touch_width_ - x * xscale_);
        default:
            return make_pair(x * xscale_, y * yscale_);
        }
    }

    std::shared_ptr<DeviceInfo> device_info_ = std::make_shared<DeviceInfo>();
};

MAA_CTRL_UNIT_NS_END
