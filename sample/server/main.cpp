#include "MaaToolKit/MaaToolKitAPI.h"
#include <iostream>

int main()
{
    std::cout << "start server..." << std::endl;
    MaaToolKitStartWebServer("127.0.0.1", 8080);
    std::cout << "Server started at http://127.0.0.1:8080/\nPress enter to close" << std::endl;
    std::cin.get();
    std::cout << "stop server..." << std::endl;
    MaaToolKitStopWebServer();
    std::cout << "Server stopped" << std::endl;
}
