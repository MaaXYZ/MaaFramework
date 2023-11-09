#include "ThriftControllerSocketAgent.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4245 4706)
#endif
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TSocket.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

ThriftControllerSocketAgent::ThriftControllerSocketAgent(const std::string& host, int port)
{
    LogFunc << VAR(host) << VAR(port);

    using namespace apache::thrift;

    std::shared_ptr<transport::TSocket> socket = std::make_shared<transport::TSocket>(host, port);

    transport_ = std::make_shared<transport::TBufferedTransport>(socket);
    auto protocol = std::make_shared<protocol::TBinaryProtocol>(transport_);
    client_ = std::make_shared<ThriftController::ThriftControllerClient>(protocol);
}

MAA_CTRL_UNIT_NS_END
