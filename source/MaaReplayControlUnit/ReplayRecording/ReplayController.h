#pragma once

#include <filesystem>

#include <meojson/json.hpp>

#include "MaaControlUnit/ControlUnitAPI.h"
#include "Record.h"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class ReplayController : public FullControlUnitAPI
{
public:
    explicit ReplayController(Recording recording);

    virtual ~ReplayController() override;

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

    virtual bool inactive() override;

public: // from FullControlUnitAPI
    virtual bool relative_move(int dx, int dy) override;
    virtual bool scroll(int dx, int dy) override;
    virtual bool
        shell(const std::string& cmd, std::string& output, std::chrono::milliseconds timeout = std::chrono::milliseconds(20000)) override;

    virtual json::object get_info() const override;

private:
    const Record* expect_record(RecordType expected_type);
    bool consume_record(const Record& record);

    template <typename T>
    const T* get_param(const Record& record);

private:
    Recording recording_;
    size_t record_index_ = 0;
    bool connected_ = false;
};

MAA_CTRL_UNIT_NS_END
