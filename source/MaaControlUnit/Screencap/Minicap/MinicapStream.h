#pragma once

#include "MinicapBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class MinicapStream : public MinicapBase
{
public:
    virtual ~MinicapStream() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from ScreencapAPI
    virtual bool init(int swidth, int sheight) override;

    virtual std::optional<cv::Mat> screencap() override;

private:
    bool read_until(size_t size);
    bool take_out(void* out, size_t size);

    Argv forward_argv_;

    std::string buffer_;
    std::shared_ptr<IOHandler> process_handle_;
    std::shared_ptr<IOHandler> stream_handle_;
};

MAA_CTRL_UNIT_NS_END
