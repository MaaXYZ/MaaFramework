#pragma once

#include <memory>

#include "MaaFramework/MaaPort.h"
#include "Utils/Boost.hpp"
#include "Utils/NonCopyable.hpp"

MAA_NS_BEGIN

class MAA_UTILS_API ChildPipeIOStream : public NonCopyButMovable
{
    static constexpr size_t kBufferSize = 128 * 1024;

public:
    ChildPipeIOStream(const std::filesystem::path& exec, const std::vector<std::string>& args);

    virtual ~ChildPipeIOStream();

public:
    bool write(std::string_view data);
    std::string read(std::chrono::seconds timeout = std::chrono::seconds(-1), size_t count = SIZE_MAX);
    int release();

private:
    boost::process::opstream pin_;
    boost::process::ipstream pout_;
    boost::process::child child_;

    std::unique_ptr<char[]> buffer_ = nullptr;
};

MAA_NS_END
