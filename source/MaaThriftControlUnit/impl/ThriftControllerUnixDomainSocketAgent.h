#pragma once

#include "ThriftControllerAgent.h"

MAA_CTRL_UNIT_NS_BEGIN

class ThriftControllerUnixDomainSocketAgent : public ThriftControllerAgent
{
public:
    ThriftControllerUnixDomainSocketAgent(const std::string& path);
    virtual ~ThriftControllerUnixDomainSocketAgent() = default;
};
MAA_CTRL_UNIT_NS_END
