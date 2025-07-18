#pragma once

#include <string>
#include <string_view>

#include <MaaAgentClient/MaaAgentClientAPI.h>

#include "./Exception.h"
#include "./Resource.h"
#include "./private/String.h"

namespace maapp
{

struct Client
{
    Client(const std::string& identifier = "")
    {
        pri::String id;
        id = identifier;
        client_ = MaaAgentClientCreateV2(id.buffer_);
    }

    Client(const Client&) = delete;

    virtual ~Client()
    {
        if (client_) {
            MaaAgentClientDestroy(client_);
        }
    }

    Client& operator=(const Client&) = delete;

    std::string identifier() const
    {
        pri::String id;
        if (!MaaAgentClientIdentifier(client_, id.buffer_)) {
            throw FunctionFailed("MaaAgentClientIdentifier");
        }
        return id;
    }

    void bind_resource(std::shared_ptr<Resource> resource)
    {
        if (!MaaAgentClientBindResource(client_, resource ? resource->resource_ : nullptr)) {
            throw FunctionFailed("MaaAgentClientBindResource");
        }
        resource_ = resource;
    }

    void connect()
    {
        if (!MaaAgentClientConnect(client_)) {
            throw FunctionFailed("MaaAgentClientConnect");
        }
    }

    void disconnect()
    {
        if (!MaaAgentClientConnect(client_)) {
            throw FunctionFailed("MaaAgentClientConnect");
        }
    }

    bool connected() const { return MaaAgentClientConnected(client_); }

    bool alive() const { return MaaAgentClientAlive(client_); }

    void set_timeout(int64_t ms)
    {
        if (!MaaAgentClientSetTimeout(client_, ms)) {
            throw FunctionFailed("MaaAgentClientSetTimeout");
        }
    }

    MaaAgentClient* client_ {};
    std::shared_ptr<Resource> resource_ {};
};

}
