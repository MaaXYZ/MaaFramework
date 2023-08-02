#include "MaaToolKit/MaaToolKitAPI.h"
#include "Server/HttpServer.h"

MaaBool MaaToolKitStartWebServer(MaaString ip, uint16_t port)
{
    return MAA_TOOLKIT_NS::HttpServer::get_instance().start(ip, port);
}

MaaBool MaaToolKitStopWebServer()
{
    return MAA_TOOLKIT_NS::HttpServer::get_instance().stop();
}
