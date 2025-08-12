#pragma once

#include <memory>

#include "Conf/Conf.h"

MAA_NS_BEGIN

class DebugServer
{
public:
    virtual ~DebugServer() = default;

    virtual bool start_service() = 0;

    virtual std::string call_remote(std::string request) = 0;

    static std::shared_ptr<DebugServer> create_server();
};

MAA_NS_END
