#pragma once

#include <unordered_set>

#include "Base/UnitBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class ScreencapAgent : public ScreencapBase
{
public:
    enum class Method
    {
        UnknownYet,
        EncodeToFileAndPull,
        Encode,
        RawWithGzip,
        RawByNetcat,
        MinicapDirect,
        MinicapStream,
        MuMuPlayerExtras,
        LDPlayerExtras,
    };

public:
    ScreencapAgent(MaaAdbScreencapMethod methods, const std::filesystem::path& agent_path);
    virtual ~ScreencapAgent() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from ScreencapBase
    virtual bool init() override;
    virtual void deinit() override;

    virtual std::optional<cv::Mat> screencap() override;

public: // from ControlUnitSink
    virtual void on_image_resolution_changed(const std::pair<int, int>& pre, const std::pair<int, int>& cur) override;
    virtual void on_app_started(const std::string& intent) override;
    virtual void on_app_stopped(const std::string& intent) override;

private:
    bool speed_test();

    std::unordered_map<Method, std::shared_ptr<ScreencapBase>> units_;
    std::shared_ptr<ScreencapBase> active_unit_;
};

std::ostream& operator<<(std::ostream& os, ScreencapAgent::Method m);

MAA_CTRL_UNIT_NS_END
