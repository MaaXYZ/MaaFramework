#pragma once

#include "Base/UnitBase.h"

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
    virtual MaaControllerFeature get_features() const override = 0;

    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

    virtual bool parse(const json::value& config) override;

    virtual bool click_key(int key) override = 0;
    virtual bool input_text(const std::string& text) override = 0;

    virtual bool key_down(int key) override = 0;
    virtual bool key_up(int key) override = 0;

    virtual bool scroll(int dx, int dy) override;

protected:
    bool read_info();

    virtual std::pair<int, int> screen_to_touch(int x, int y) = 0;
    virtual std::pair<int, int> screen_to_touch(double x, double y) = 0;

    virtual std::string type_name() const;

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

private:
    bool request_display_info();
    std::shared_ptr<DeviceInfo> device_info_ = std::make_shared<DeviceInfo>();
};

MAA_CTRL_UNIT_NS_END
