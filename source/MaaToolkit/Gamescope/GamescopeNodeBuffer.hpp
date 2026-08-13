#pragma once

#include <cstdint>
#include <string>

#include "API/MaaToolkitBufferTypes.hpp"
#include "Common/Conf.h"
#include "MaaUtils/Buffer/ListBuffer.hpp"

MAA_TOOLKIT_NS_BEGIN

struct GamescopeNode
{
    uint32_t id = 0;
    std::string name;
};

class GamescopeNodeBuffer : public MaaToolkitGamescopeNode
{
public:
    GamescopeNodeBuffer(uint32_t id, std::string name)
        : id_(id)
        , name_(std::move(name))
    {
    }

    virtual ~GamescopeNodeBuffer() override = default;

    virtual uint32_t id() const override { return id_; }

    virtual const std::string& name() const override { return name_; }

private:
    uint32_t id_ = 0;
    std::string name_;
};

MAA_TOOLKIT_NS_END

struct MaaToolkitGamescopeNodeList : public MAA_NS::ListBuffer<MAA_TOOLKIT_NS::GamescopeNodeBuffer>
{
    virtual ~MaaToolkitGamescopeNodeList() override = default;
};
