#pragma once

#include <memory>

#include "IOHandler.h"
#include "Utils/Boost.hpp"

MAA_NS_BEGIN

class PipeIOHandler : public IOHandler
{
    static constexpr size_t kBufferSize = 4096;

public:
    PipeIOHandler(const std::filesystem::path& exec, const std::vector<std::string>& args);
    virtual ~PipeIOHandler() override;

public: // from IOHandler
    virtual bool write(const std::string& data) override;
    virtual std::string read(std::chrono::milliseconds timeout, size_t max_length = SIZE_MAX) override;

private:
    boost::process::opstream pin_;
    boost::process::ipstream pout_;
    boost::process::child child_;

    std::unique_ptr<char[]> buffer_;
};

MAA_NS_END
