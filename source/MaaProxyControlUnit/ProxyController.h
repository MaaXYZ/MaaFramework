#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>

#include "ControlUnit/ControlUnitAPI.h"
#include "ControlUnit/RecordTypes.h"
#include "MaaUtils/NoWarningCVMat.hpp"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class ProxyController : public AdbControlUnitAPI
{
public:
    ProxyController(ControlUnitAPI* inner, std::filesystem::path dump_dir);
    virtual ~ProxyController() override;

public: // from ControlUnitAPI
    virtual bool connect() override;
    virtual bool connected() const override;

    virtual bool request_uuid(/*out*/ std::string& uuid) override;
    virtual MaaControllerFeature get_features() const override;

    virtual bool start_app(const std::string& intent) override;
    virtual bool stop_app(const std::string& intent) override;

    virtual bool screencap(/*out*/ cv::Mat& image) override;

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

public: // from AdbControlUnitAPI
    virtual bool find_device(/*out*/ std::vector<std::string>& devices) override;
    virtual bool
        shell(const std::string& cmd, std::string& output, std::chrono::milliseconds timeout = std::chrono::milliseconds(20000)) override;

private:
    void write_record(const json::value& record);

    template <typename ParamT>
    bool forward_and_record(RecordType type, const ParamT& param, std::function<bool()> action_fn);

    RecordLine make_line(RecordType type, bool success, int64_t timestamp, int cost);

private:
    ControlUnitAPI* inner_;
    std::filesystem::path dump_dir_;

    std::mutex recording_mutex_;
    std::ofstream record_file_;
    size_t screencap_count_ = 0;
    std::chrono::steady_clock::time_point recording_start_;
};

MAA_CTRL_UNIT_NS_END
