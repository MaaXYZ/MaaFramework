#pragma once

#include "Common/MaaTypes.h"

MAA_AGENT_SERVER_NS_BEGIN

class RemoteObject
{
public:
    virtual ~RemoteObject() = default;

    template <typename ResponseT, typename RequestT>
    std::optional<ResponseT> send_and_recv(const RequestT& req)
    {
    }

private:
    bool send(const json::value& j);
    std::optional<json::value> recv();
};

MAA_AGENT_SERVER_NS_END
