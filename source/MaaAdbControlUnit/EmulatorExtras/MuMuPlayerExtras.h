#pragma once

#ifdef _WIN32

#include <optional>

#include "Mumu/external_renderer_ipc/external_renderer_ipc.h"

#include "Base/UnitBase.h"
#include "MaaUtils/LibraryHolder.h"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class MuMuPlayerExtras
    : public LibraryHolder<MuMuPlayerExtras>
    , public ScreencapBase
    , public InputBase
{
public:
    virtual ~MuMuPlayerExtras() override;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

    virtual void on_image_resolution_changed(const std::pair<int, int>& pre, const std::pair<int, int>& cur) override;

public: // from ScreencapBase
    virtual bool init() override;

    virtual std::optional<cv::Mat> screencap() override;

public: // from InputBase
    virtual MaaControllerFeature get_features() const override;

    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;
    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

    virtual bool click_key(int key) override;
    virtual bool input_text(const std::string& text) override;

    virtual bool key_down(int key) override;
    virtual bool key_up(int key) override;

    virtual bool scroll(int dx, int dy) override;

public: // from ControlUnitSink
    virtual void on_app_started(const std::string& intent) override;
    virtual void on_app_stopped(const std::string& intent) override;

private:
    static int android_keycode_to_linux_key_code(int key);

private:
    bool load_mumu_library();
    bool connect_mumu();
    bool init_screencap();
    void disconnect_mumu();
    void set_app_package(const std::string& package, int cloned_index);
    void clear_display_id();
    int get_display_id();

private:
    std::filesystem::path mumu_path_;
    std::filesystem::path lib_path_;
    int mumu_index_ = 0;

    inline static std::map<int, int> s_mumu_handle_refs_;
    int mumu_handle_ = 0;

    std::string mumu_app_package_;
    int mumu_app_cloned_index_ = 0;
    std::optional<int> mumu_display_id_cache_;

    int display_width_ = 0;
    int display_height_ = 0;
    std::vector<unsigned char> display_buffer_;

private:
    inline static const std::string kConnectFuncName = "nemu_connect";
    inline static const std::string kDisconnectFuncName = "nemu_disconnect";
    inline static const std::string kCaptureDisplayFuncName = "nemu_capture_display";
    inline static const std::string kInputTextFuncName = "nemu_input_text";
    inline static const std::string kInputEventTouchDownFuncName = "nemu_input_event_finger_touch_down";
    inline static const std::string kInputEventTouchUpFuncName = "nemu_input_event_finger_touch_up";
    inline static const std::string kInputEventKeyDownFuncName = "nemu_input_event_key_down";
    inline static const std::string kInputEventKeyUpFuncName = "nemu_input_event_key_up";
    inline static const std::string kGetDisplayIdFuncName = "nemu_get_display_id";

private:
    boost::function<decltype(nemu_connect)> connect_func_;
    boost::function<decltype(nemu_disconnect)> disconnect_func_;
    boost::function<decltype(nemu_capture_display)> capture_display_func_;
    boost::function<decltype(nemu_input_text)> input_text_func_;
    boost::function<decltype(nemu_input_event_finger_touch_down)> input_event_touch_down_func_;
    boost::function<decltype(nemu_input_event_finger_touch_up)> input_event_touch_up_func_;
    boost::function<decltype(nemu_input_event_key_down)> input_event_key_down_func_;
    boost::function<decltype(nemu_input_event_key_up)> input_event_key_up_func_;
    boost::function<decltype(nemu_get_display_id)> get_display_id_func_;
};

MAA_CTRL_UNIT_NS_END

#endif
