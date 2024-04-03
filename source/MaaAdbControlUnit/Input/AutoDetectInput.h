#pragma once

#include "Base/UnitBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class AutoDetectInput
    : public TouchInputBase
    , public KeyInputBase
{
public:
    enum class TouchMethod
    {
        UnknownYet,
        Maatouch,
        Minitouch,
        AdbTap,
    };

    enum class KeyMethod
    {
        UnknownYet,
        Maatouch,
        AdbKey,
    };

public:
    AutoDetectInput(
        const std::filesystem::path& maatouch_path,
        const std::filesystem::path& minitouch_path);
    virtual ~AutoDetectInput() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from TouchInputAPI
    virtual bool init(int swidth, int sheight, int orientation) override;
    virtual void deinit() override;
    virtual bool set_wh(int swidth, int sheight, int orientation) override;

    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

public: // from KeyInputAPI
    virtual bool init() override;
    virtual bool press_key(int key) override;
    virtual bool input_text(const std::string& text) override;

private:
    std::vector<std::pair<TouchMethod, std::shared_ptr<TouchInputBase>>> touch_units_;
    std::vector<std::pair<KeyMethod, std::shared_ptr<KeyInputBase>>> key_units_;

    std::shared_ptr<TouchInputBase> available_touch_ = nullptr;
    std::shared_ptr<KeyInputBase> available_key_ = nullptr;
};

MAA_CTRL_UNIT_NS_END
