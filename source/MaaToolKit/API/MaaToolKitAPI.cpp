#include "MaaToolKit/MaaToolKitAPI.h"
#include "Server/HttpServer.h"
#include "Server/WsServer.h"

MaaBool MaaToolKitStartWebServer(MaaString ip, MaaPort port)
{
    return MAA_TOOLKIT_NS::HttpServer::get_instance().start(ip, port) &&
           MAA_TOOLKIT_NS::WsServer::get_instance().start(ip, port + 1);
}

MaaBool MaaToolKitStopWebServer()
{
    return MAA_TOOLKIT_NS::HttpServer::get_instance().stop() && MAA_TOOLKIT_NS::WsServer::get_instance().stop();
}
