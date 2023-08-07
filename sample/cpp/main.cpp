#include "MaaToolKit/MaaToolKitAPI.h"

int main()
{
    MaaToolKitInit();

    auto* handle = MaaToolKitCurrentConfig();

    MaaToolKitPostAllTask(handle);
    MaaToolKitWaitAllTask(handle);

    MaaToolKitUninit();

    return 0;
}
