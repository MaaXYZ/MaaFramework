#include "MaaToolKit/MaaToolKitAPI.h"
#include "Server/Server.h"

MaaBool MaaToolKitStartWebServer(MaaString ip, MaaString port)
{
    return MaaNS::ToolKitNS::HttpServer::get_instance().start(ip, port);
}

MaaBool MaaToolKitStopWebServer()
{
    return MaaNS::ToolKitNS::HttpServer::get_instance().stop();
}
