#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>

#include "Common/MaaTypes.h"
#include "ControlUnit/RecordTypes.h"
#include "MaaUtils/NoWarningCVMat.hpp"
#include "Utils/EventDispatcher.hpp"

#include "Common/Conf.h"

MAA_CTRL_NS_BEGIN

class ProxyController : public MaaController
{
public:
    ProxyController(MaaController* inner, std::filesystem::path dump_dir);
    virtual ~ProxyController() override;

public: // MaaController
    virtual bool set_option(MaaCtrlOption key, MaaOptionValue value, MaaOptionValueSize val_size) override;

    virtual MaaCtrlId post_connection() override;
    virtual MaaCtrlId post_click(int x, int y, int contact, int pressure) override;
    virtual MaaCtrlId post_swipe(int x1, int y1, int x2, int y2, int duration, int contact, int pressure) override;
    virtual MaaCtrlId post_click_key(int keycode) override;
    virtual MaaCtrlId post_input_text(const std::string& text) override;
    virtual MaaCtrlId post_start_app(const std::string& intent) override;
    virtual MaaCtrlId post_stop_app(const std::string& intent) override;
    virtual MaaCtrlId post_screencap() override;

    virtual MaaCtrlId post_touch_down(int contact, int x, int y, int pressure) override;
    virtual MaaCtrlId post_touch_move(int contact, int x, int y, int pressure) override;
    virtual MaaCtrlId post_touch_up(int contact) override;

    virtual MaaCtrlId post_key_down(int keycode) override;
    virtual MaaCtrlId post_key_up(int keycode) override;

    virtual MaaCtrlId post_scroll(int dx, int dy) override;

    virtual MaaCtrlId post_shell(const std::string& cmd, int64_t timeout = 20000) override;

    virtual MaaStatus status(MaaCtrlId ctrl_id) const override;
    virtual MaaStatus wait(MaaCtrlId ctrl_id) const override;
    virtual bool connected() const override;
    virtual bool running() const override;

    virtual cv::Mat cached_image() const override;
    virtual std::string cached_shell_output() const override;
    virtual std::string get_uuid() override;

    virtual bool get_resolution(int32_t& width, int32_t& height) const override;

    virtual MaaSinkId add_sink(MaaEventCallback callback, void* trans_arg) override;
    virtual void remove_sink(MaaSinkId sink_id) override;
    virtual void clear_sinks() override;

private:
    void write_record(const json::value& record);

    template <typename ParamT>
    MaaCtrlId forward_and_record(const std::string& type, const ParamT& param, std::function<MaaCtrlId()> post_fn);

    MAA_CTRL_UNIT_NS::RecordLine make_line(const std::string& type, bool success, int64_t timestamp, int cost);

    static void MAA_CALL inner_sink_callback(void* handle, const char* message, const char* details_json, void* trans_arg);

private:
    MaaController* inner_;
    std::filesystem::path dump_dir_;

    EventDispatcher notifier_;
    MaaSinkId inner_sink_id_ = MaaInvalidId;

    std::mutex recording_mutex_;
    std::ofstream record_file_;
    size_t screencap_count_ = 0;
    std::chrono::steady_clock::time_point recording_start_;
};

MAA_CTRL_NS_END
