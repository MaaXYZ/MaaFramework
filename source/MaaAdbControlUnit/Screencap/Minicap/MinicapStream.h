#pragma once

#include "MinicapBase.h"

#include <condition_variable>
#include <thread>

MAA_CTRL_UNIT_NS_BEGIN

class MinicapStream : public MinicapBase
{
public:
    using MinicapBase::MinicapBase;

    virtual ~MinicapStream() override;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from ScreencapAPI
    virtual bool init(int swidth, int sheight) override;

    virtual std::optional<cv::Mat> screencap() override;

private:
    bool read_until(std::string& buffer, size_t size);
    bool take_out(void* out, size_t size);
    void working_thread();

    Argv forward_argv_;

    bool quit_ = true;
    std::mutex mutex_;
    cv::Mat image_;
    std::condition_variable cond_;
    std::thread pull_thread_;

    std::shared_ptr<IOHandler> process_handle_;
    std::shared_ptr<IOHandler> stream_handle_;
};

MAA_CTRL_UNIT_NS_END
