#pragma once

#include "Mumu/external_renderer_ipc/external_renderer_ipc.h"

#include "Base/UnitBase.h"
#include "LibraryHolder/LibraryHolder.h"

MAA_CTRL_UNIT_NS_BEGIN

class MumuExternalRendererIpc
    : public LibraryHolder<MumuExternalRendererIpc>
    , public ScreencapBase
    , public TouchInputBase
    , public KeyInputBase
{
public:
    virtual ~MumuExternalRendererIpc() override;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from ScreencapAPI
    virtual bool init() override;
    virtual void deinit() override;

    virtual std::optional<cv::Mat> screencap() override;

public: // from TouchInputAPI
    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

public: // from KeyInputAPI
    virtual bool press_key(int key) override;
    virtual bool input_text(const std::string& text) override;

private:
    bool load_mumu_library();
    bool connect_mumu();
    bool init_screencap();
    void disconnect_mumu();

private:
    std::filesystem::path mumu_path_;
    std::filesystem::path lib_path_;
    int mumu_index_ = 0;
    unsigned int mumu_display_id_ = 0;
    int mumu_handle_ = 0;

    int display_width_ = 0;
    int display_height_ = 0;
    std::vector<unsigned char> display_buffer_;

private:
    inline static const std::string kConnectFuncName = "nemu_connect";
    inline static const std::string kDisconnectFuncName = "nemu_disconnect";
    inline static const std::string kCaptureDisplayFuncName = "nemu_capture_display";
    inline static const std::string kInputTextFuncName = "nemu_input_text";
    inline static const std::string kInputEventTouchDownFuncName = "nemu_input_event_touch_down";
    inline static const std::string kInputEventTouchUpFuncName = "nemu_input_event_touch_up";
    inline static const std::string kInputEventKeyDownFuncName = "nemu_input_event_key_down";
    inline static const std::string kInputEventKeyUpFuncName = "nemu_input_event_key_up";

private:
    boost::function<decltype(nemu_connect)> connect_func_;
    boost::function<decltype(nemu_disconnect)> disconnect_func_;
    boost::function<decltype(nemu_capture_display)> capture_display_func_;
    boost::function<decltype(nemu_input_text)> input_text_func_;
    boost::function<decltype(nemu_input_event_touch_down)> input_event_touch_down_func_;
    boost::function<decltype(nemu_input_event_touch_up)> input_event_touch_up_func_;
    boost::function<decltype(nemu_input_event_key_down)> input_event_key_down_func_;
    boost::function<decltype(nemu_input_event_key_up)> input_event_key_up_func_;
};

MAA_CTRL_UNIT_NS_END
