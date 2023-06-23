#pragma once

#include "MinicapBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class MinicapStream : public MinicapBase
{
public:
    bool parse(const json::value& config) override;

    bool init(int w, int h) override;

    std::optional<cv::Mat> screencap() override;

private:
    void read_until(size_t size);
    void take_out(void* out, size_t size);

    Argv forward_argv_;

    std::string buffer_;
    std::shared_ptr<IOHandler> process_handle_;
    std::shared_ptr<IOHandler> stream_handle_;
};

MAA_CTRL_UNIT_NS_END
