#pragma once
#include <utility>

#include "Base/MemfdBuffer.h"
#include "Base/UnitBase.h"
#include "Common/Conf.h"
#include "Wayland/WaylandClient.h"
#include "Wayland/WaylandHelper.h"

MAA_CTRL_UNIT_NS_BEGIN

class WlrScreencap : public ScreencapBase
{
public:
    explicit WlrScreencap(std::shared_ptr<WaylandClient> client)
        : client_(std::move(client))
        , screencopy_manager_(client_->get_screencopy_manager())
        , shm_(client_->get_shm())
        , output_(client_->get_output())
    {
    }

    virtual ~WlrScreencap() override = default;

public:
    virtual std::optional<cv::Mat> screencap() override;

private:
    bool check_buffer(int format, int width, int height, int stride) const;
    bool create_buffer(int format, int width, int height, int stride);
    bool close_buffer();

private:
    std::shared_ptr<WaylandClient> client_;
    std::shared_ptr<zwlr_screencopy_manager_v1> screencopy_manager_;
    std::shared_ptr<wl_shm> shm_;
    std::shared_ptr<wl_output> output_;
    bool capture_waiting_ = false;
    bool capture_successful_ = false;
    std::unique_ptr<MemfdBuffer> buffer_;
    std::unique_ptr<wl_shm_pool> shm_pool_;
    std::unique_ptr<wl_buffer> buffer_obj_;
    int buffer_width_ = 0;
    int buffer_height_ = 0;
    int buffer_format_ = 0;
    int buffer_stride_ = 0;
};

MAA_CTRL_UNIT_NS_END
