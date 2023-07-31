#include "MaaToolKit/MaaToolKitAPI.h"
#include "Server/Server.h"

MaaBool MaaToolKitStartWebServer(MaaString ip, MaaPort port)
{
    return MAA_TOOLKIT_NS::HttpServer::get_instance().start(ip, port);
}

MaaBool MaaToolKitStopWebServer()
{
    return MAA_TOOLKIT_NS::HttpServer::get_instance().stop();
}
