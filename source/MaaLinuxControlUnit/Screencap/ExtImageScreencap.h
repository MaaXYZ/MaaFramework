#pragma once
#include <vector>

#include "Base/MemfdBuffer.h"
#include "Base/UnitBase.h"
#include "Common/Conf.h"
#include "Wayland/WaylandClient.h"
#include "Wayland/WaylandHelper.h"

MAA_CTRL_UNIT_NS_BEGIN

class ExtImageScreencap : public ScreencapBase
{
public:
    explicit ExtImageScreencap(std::shared_ptr<WaylandClient> client)
        : client_(std::move(client))
        , source_manager_(client_->get_ext_image_source_manager())
        , capture_manager_(client_->get_ext_image_copy_manager())
        , shm_(client_->get_shm())
        , output_(client_->get_output())
    {
    }

    virtual ~ExtImageScreencap() override = default;

public:
    virtual bool screencap(/*out*/ cv::Mat& image) override;

private:
    bool check_buffer(int format, int width, int height) const;
    bool create_buffer(int format, int width, int height);
    bool close_buffer();

private:
    std::shared_ptr<WaylandClient> client_;
    std::shared_ptr<ext_output_image_capture_source_manager_v1> source_manager_;
    std::shared_ptr<ext_image_copy_capture_manager_v1> capture_manager_;
    std::shared_ptr<wl_shm> shm_;
    std::shared_ptr<wl_output> output_;

    // Per-capture session state. A fresh session is created for every capture
    // so each frame is the session's first one, which the compositor must copy
    // immediately; later frames may block until the source content changes.
    bool constraints_ready_ = false;
    bool session_stopped_ = false;
    bool capture_waiting_ = false;
    bool capture_successful_ = false;
    bool buffer_stale_ = false;
    uint32_t capture_width_ = 0;
    uint32_t capture_height_ = 0;
    std::vector<uint32_t> shm_formats_;

    std::unique_ptr<MemfdBuffer> buffer_;
    std::unique_ptr<wl_shm_pool> shm_pool_;
    std::unique_ptr<wl_buffer> buffer_obj_;
    int buffer_width_ = 0;
    int buffer_height_ = 0;
    int buffer_format_ = 0;
    int buffer_stride_ = 0;
};

MAA_CTRL_UNIT_NS_END
