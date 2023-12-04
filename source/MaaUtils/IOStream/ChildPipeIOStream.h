#pragma once

#include <memory>

#include "Utils/Boost.hpp"

MAA_NS_BEGIN

class ChildPipeIOStream
{
    static constexpr size_t kBufferSize = 128 * 1024;

public:
    ChildPipeIOStream(const std::filesystem::path& exec, const std::vector<std::string>& args);
    virtual ~ChildPipeIOStream();

public:
    bool write(std::string_view data);
    std::string read(std::chrono::milliseconds timeout, size_t count = SIZE_MAX);

private:
    boost::process::opstream pin_;
    boost::process::ipstream pout_;
    boost::process::child child_;

    std::unique_ptr<char[]> buffer_ = nullptr;
};

MAA_NS_END
