#pragma once

#include "ThriftControllerAgent.h"

MAA_CTRL_UNIT_NS_BEGIN

class ThriftControllerSocketAgent : public ThriftControllerAgent
{
public:
    ThriftControllerSocketAgent(const std::string& host, int port);
    virtual ~ThriftControllerSocketAgent() = default;
};
MAA_CTRL_UNIT_NS_END
