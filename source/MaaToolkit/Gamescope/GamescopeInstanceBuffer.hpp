#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include "API/MaaToolkitBufferTypes.hpp"
#include "Common/Conf.h"
#include "MaaUtils/Buffer/ListBuffer.hpp"

MAA_TOOLKIT_NS_BEGIN

struct GamescopeInstance
{
    uint32_t display_no = 0;
    uint32_t pipewire_node_id = 0;
    std::string eis_socket_path;
};

class GamescopeInstanceBuffer : public MaaToolkitGamescopeInstance
{
public:
    explicit GamescopeInstanceBuffer(GamescopeInstance instance)
        : instance_(std::move(instance))
    {
    }

    virtual ~GamescopeInstanceBuffer() override = default;

    virtual uint32_t display_no() const override { return instance_.display_no; }

    virtual uint32_t pipewire_node_id() const override { return instance_.pipewire_node_id; }

    virtual const std::string& eis_socket_path() const override { return instance_.eis_socket_path; }

private:
    GamescopeInstance instance_;
};

MAA_TOOLKIT_NS_END

struct MaaToolkitGamescopeInstanceList : public MAA_NS::ListBuffer<MAA_TOOLKIT_NS::GamescopeInstanceBuffer>
{
    virtual ~MaaToolkitGamescopeInstanceList() override = default;
};
