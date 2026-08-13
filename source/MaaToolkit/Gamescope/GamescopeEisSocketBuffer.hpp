#pragma once

#include <string>

#include "API/MaaToolkitBufferTypes.hpp"
#include "Common/Conf.h"
#include "MaaUtils/Buffer/ListBuffer.hpp"

MAA_TOOLKIT_NS_BEGIN

struct GamescopeEisSocket
{
    std::string path;
    unsigned long display_no = 0;
};

class GamescopeEisSocketBuffer : public MaaToolkitGamescopeEisSocket
{
public:
    explicit GamescopeEisSocketBuffer(std::string path)
        : path_(std::move(path))
    {
    }

    virtual ~GamescopeEisSocketBuffer() override = default;

    virtual const std::string& path() const override { return path_; }

private:
    std::string path_;
};

MAA_TOOLKIT_NS_END

struct MaaToolkitGamescopeEisSocketList : public MAA_NS::ListBuffer<MAA_TOOLKIT_NS::GamescopeEisSocketBuffer>
{
    virtual ~MaaToolkitGamescopeEisSocketList() override = default;
};
