#pragma once

#include <chrono>
#include <memory>
#include <utility>

#include "Base/ForegroundUtils.h"
#include "Base/UnitBase.h"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class ForegroundScreencap : public ScreencapBase
{
public:
    ForegroundScreencap(HWND hwnd, std::shared_ptr<ScreencapBase> inner)
        : hwnd_(hwnd)
        , inner_(std::move(inner))
    {
    }

    virtual ~ForegroundScreencap() override = default;

public: // from ScreencapBase
    virtual std::optional<cv::Mat> screencap() override
    {
        if (!ensure_foreground_with_cooldown()) {
            return std::nullopt;
        }
        return inner_->screencap();
    }

    virtual void inactive() override { inner_->inactive(); }

private:
    bool ensure_foreground_with_cooldown()
    {
        constexpr auto kForegroundRecoveryInterval = std::chrono::seconds(5);

        if (hwnd_ == GetForegroundWindow()) {
            return true;
        }

        auto now = std::chrono::steady_clock::now();
        if (last_foreground_attempt_ != std::chrono::steady_clock::time_point::min()
            && now - last_foreground_attempt_ < kForegroundRecoveryInterval) {
            return false;
        }

        last_foreground_attempt_ = now;
        ensure_foreground_and_topmost(hwnd_);
        return hwnd_ == GetForegroundWindow();
    }

    HWND hwnd_ = nullptr;
    std::shared_ptr<ScreencapBase> inner_ = nullptr;
    std::chrono::steady_clock::time_point last_foreground_attempt_ = std::chrono::steady_clock::time_point::min();
};

MAA_CTRL_UNIT_NS_END
