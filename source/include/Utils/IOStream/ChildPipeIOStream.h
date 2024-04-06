#pragma once

#include <memory>

#include "IOStream.h"
#include "MaaFramework/MaaPort.h"

MAA_NS_BEGIN

class MAA_UTILS_API ChildPipeIOStream : public IOStream
{
public:
    ChildPipeIOStream(const std::filesystem::path& exec, const std::vector<std::string>& args);

#ifdef _WIN32
    ChildPipeIOStream(const std::filesystem::path& exec, const std::vector<std::wstring>& wargs);
#endif

    // NonCopyButMovable
    // https://stackoverflow.com/questions/29289956/c11-virtual-destructors-and-auto-generation-of-move-special-functions
    ChildPipeIOStream(const ChildPipeIOStream&) = delete;
    ChildPipeIOStream(ChildPipeIOStream&&) = default;
    ChildPipeIOStream& operator=(const ChildPipeIOStream&) = delete;
    ChildPipeIOStream& operator=(ChildPipeIOStream&&) = default;

    virtual ~ChildPipeIOStream() override;

public:
    virtual bool write(std::string_view data) override;

    virtual bool release() override;
    virtual bool is_open() const override;

protected:
    virtual std::string read_once(size_t max_count) override;

private:
    using os_string = std::filesystem::path::string_type;

    ChildPipeIOStream(const std::filesystem::path& exec, const std::vector<os_string>& args, bool);

private:
    std::filesystem::path exec_;
    std::vector<os_string> args_;

    boost::process::ipstream pin_;
    boost::process::opstream pout_;
    boost::process::child child_;

    std::unique_ptr<char[]> buffer_ = nullptr;
};

MAA_NS_END
