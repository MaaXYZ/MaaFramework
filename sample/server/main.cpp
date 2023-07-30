#include "MaaToolKit/MaaToolKitAPI.h"
#include <stdio.h>

int main()
{
    MaaToolKitStartWebServer("127.0.0.1", "8080");
    getchar();
    MaaToolKitStopWebServer();
}