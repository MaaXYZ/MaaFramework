#include "MaaRpc/MaaRpc.h"
#include <iostream>

int main()
{
    std::string server_address("0.0.0.0:8080");
    MaaRpcStart(server_address.c_str());
    std::cout << "Server listening on " << server_address << std::endl;
    MaaRpcWait();
}
