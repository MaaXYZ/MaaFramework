#pragma once

#include "PrintWindowScreencap.h"
#include "PseudoMinimizeHelper.h"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class PrintWindowWithPseudoMinimizeScreencap : public ScreencapBase
{
public:
    explicit PrintWindowWithPseudoMinimizeScreencap(HWND hwnd)
        : inner_(hwnd)
        , helper_(hwnd)
    {
        helper_.start();
    }

    virtual ~PrintWindowWithPseudoMinimizeScreencap() override = default;

public: // from ScreencapBase
    virtual std::optional<cv::Mat> screencap() override
    {
        helper_.ensure_not_minimized();
        return inner_.screencap();
    }

    virtual void inactive() override
    {
        // revert_pseudo_minimize() 只恢复扩展样式与不透明度，不会把窗口收回最小化状态。
        // 这里恢复后窗口将完整显示出来，用户原本的最小化意图会丢失，
        // stop() 中为此额外补了一次 ShowWindow(SW_MINIMIZE)
        if (helper_.is_pseudo_minimized()) {
            helper_.revert_pseudo_minimize();
        }
    }

private:
    PrintWindowScreencap inner_;
    PseudoMinimizeHelper helper_;
};

MAA_CTRL_UNIT_NS_END
