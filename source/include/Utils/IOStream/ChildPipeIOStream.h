#pragma once

#include <memory>

#include "MaaFramework/MaaPort.h"
#include "Utils/Boost.hpp"
#include "Utils/NonCopyable.hpp"

MAA_NS_BEGIN

class MAA_UTILS_API ChildPipeIOStream : public NonCopyButMovable
{
    using duration_t = std::chrono::milliseconds;

    static constexpr size_t kBufferSize = 128 * 1024;

public:
    ChildPipeIOStream(const std::filesystem::path& exec, const std::vector<std::string>& args);

    virtual ~ChildPipeIOStream();

public:
    bool write(std::string_view data);

    std::string read(duration_t timeout = duration_t::max());
    std::string read_some(size_t count, duration_t timeout = duration_t::max());
    std::string read_until(std::string_view delimiter, duration_t timeout = duration_t::max());

    int release();
    bool is_open() const;

private:
    boost::process::ipstream pin_;
    boost::process::opstream pout_;
    boost::process::child child_;

    std::unique_ptr<char[]> buffer_ = nullptr;
};

MAA_NS_END
