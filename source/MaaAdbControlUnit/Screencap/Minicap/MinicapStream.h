#pragma once

#include "MinicapBase.h"

#include <condition_variable>
#include <thread>

#include "Utils/IOStream/ChildPipeIOStream.h"
#include "Utils/IOStream/SockIOStream.h"

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
    std::optional<std::string> read(size_t count);

    void working_thread();

    ProcessArgvGenerator forward_argv_;
    int port_ = 0;

    bool quit_ = true;
    std::mutex mutex_;
    cv::Mat image_;
    std::condition_variable cond_;
    std::thread pull_thread_;

    std::shared_ptr<ChildPipeIOStream> process_handle_;
    std::shared_ptr<SockIOStream> stream_handle_;
};

MAA_CTRL_UNIT_NS_END
