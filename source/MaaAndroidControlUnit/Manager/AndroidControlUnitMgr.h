#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include <jni.h>
#include <meojson/json.hpp>

#include "ControlUnit/AndroidControlUnitAPI.h"
#include "MaaUtils/Dispatcher.hpp"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class AndroidControlUnitMgr
    : public AndroidControlUnitAPI
    , public Dispatcher<ControlUnitSink>
{
public:
    AndroidControlUnitMgr(MaaAndroidScreencapMethod screencap_methods, MaaAndroidInputMethod input_methods);
    virtual ~AndroidControlUnitMgr() override = default;

public: // from ControlUnitAPI
    virtual bool connect() override;

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

private:
    // JNI helpers
    JNIEnv* ensure_env();
    jclass bridge_class();

    template <typename Callable>
    bool call_bool(const char* name, const char* sig, Callable&& caller);

private:
    const MaaAndroidScreencapMethod screencap_methods_ = MaaAndroidScreencapMethod_None;
    const MaaAndroidInputMethod input_methods_ = MaaAndroidInputMethod_None;

    JavaVM* vm_ = nullptr;
    jclass bridge_cls_ = nullptr;
};

MAA_CTRL_UNIT_NS_END
