#pragma once

#include <filesystem>
#include <optional>
#include <unordered_map>

#include "Common/Conf.h"
#include "General/AndroidExternalLib.h"
#include "MaaControlUnit/ControlUnitAPI.h"
#include "MaaUtils/NoWarningCV.hpp"

MAA_CTRL_UNIT_NS_BEGIN

struct AndroidNativeControlUnitConfig
{
    std::filesystem::path library_path;
    int touch_width = 0;
    int touch_height = 0;
    int display_id = 0;
    bool force_stop = false;
};

class AndroidNativeControlUnitMgr : public AndroidNativeControlUnitAPI
{
public:
    explicit AndroidNativeControlUnitMgr(AndroidNativeControlUnitConfig config);
    ~AndroidNativeControlUnitMgr() override;

    // from ControlUnitAPI
    bool connect() override;
    bool connected() const override;

    bool request_uuid(std::string& uuid) override;
    MaaControllerFeature get_features() const override;

    bool start_app(const std::string& intent) override;
    bool stop_app(const std::string& intent) override;

    bool screencap(cv::Mat& image) override;

    bool click(int x, int y) override;
    bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    bool touch_down(int contact, int x, int y, int pressure) override;
    bool touch_move(int contact, int x, int y, int pressure) override;
    bool touch_up(int contact) override;

    bool click_key(int key) override;
    bool input_text(const std::string& text) override;

    bool key_down(int key) override;
    bool key_up(int key) override;

    bool inactive() override;

    json::object get_info() const override;

private:
    static bool validate_contact(int contact);
    bool dispatch_input_message(AndroidNativeNS::MethodParam param) const;
    bool normalize_touch_point(int raw_x, int raw_y, cv::Point& mapped) const;
    cv::Point get_touch_up_point(int contact) const;

    AndroidNativeControlUnitConfig config_;
    std::optional<AndroidNativeNS::AndroidExternalFunctions> funcs_;
    bool connected_ = false;
    std::unordered_map<int, cv::Point> last_touch_points_;
};

MAA_CTRL_UNIT_NS_END
