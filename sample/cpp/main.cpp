#include "MaaToolKit/MaaToolKitAPI.h"

int main()
{
    MaaToolKitConfigInit();

    auto* handle = MaaToolKitCurrentConfig();

    MaaToolKitPostAllTask(handle);
    MaaToolKitWaitAllTask(handle);

    MaaToolKitConfigUninit();

    return 0;
}
