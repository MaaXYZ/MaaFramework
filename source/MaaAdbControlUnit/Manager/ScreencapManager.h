#pragma once

#include <unordered_set>

#include "Base/UnitBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class ScreencapManager : public ScreencapBase
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
        MumuExternalRendererIpc,
    };
    using MethodSet = std::unordered_set<Method>;

public:
    ScreencapManager(const MethodSet& screencap_methods, const std::filesystem::path& agent_path);
    virtual ~ScreencapManager() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from ScreencapBase
    virtual bool init() override;
    virtual void deinit() override;

    virtual std::optional<cv::Mat> screencap() override;

private:
    bool speed_test();

    std::unordered_map<Method, std::shared_ptr<ScreencapBase>> units_;
    Method method_ = Method::UnknownYet;
};

std::ostream& operator<<(std::ostream& os, ScreencapManager::Method m);

MAA_CTRL_UNIT_NS_END
