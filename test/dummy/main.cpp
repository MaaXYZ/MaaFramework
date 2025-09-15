#include <MaaFramework/MaaAPI.h>
#include <MaaToolkit/MaaToolkitAPI.h>

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>

int main()
{
    MaaToolkitDesktopWindowList* windowList = MaaToolkitDesktopWindowListCreate();
    MaaToolkitDesktopWindowFindAll(windowList);
    auto count = MaaToolkitDesktopWindowListSize(windowList);

    uint32_t handle = 0;
    for (size_t i = 0; i < count; i++) {
        auto wnd = MaaToolkitDesktopWindowListAt(windowList, i);
        std::string name = MaaToolkitDesktopWindowGetWindowName(wnd);
        std::string cls_name = MaaToolkitDesktopWindowGetClassName(wnd);
        std::cout << i << " name: " << name << " class: " << cls_name << std::endl;
        if (!handle && cls_name == "Code") {
            handle = reinterpret_cast<uint64_t>(MaaToolkitDesktopWindowGetHandle(wnd));
        }
    }

    auto ctrl = MaaMacControllerCreate(handle, MaaMacScreencapMethod_CGWindowList, MaaMacInputMethod_CGEvent, 0, 0);
    std::cout << ctrl << std::endl;

    auto actC = MaaControllerPostConnection(ctrl);
    MaaControllerWait(ctrl, actC);

    auto actS = MaaControllerPostScreencap(ctrl);
    MaaControllerWait(ctrl, actS);
    auto h = MaaImageBufferCreate();
    MaaControllerCachedImage(ctrl, h);
    auto imgSize = MaaImageBufferGetEncodedSize(h);
    auto imgData = MaaImageBufferGetEncoded(h);
    FILE* file = fopen("1.png", "wb");
    fwrite(imgData, imgSize, 1, file);
    fclose(file);

    // auto act = MaaControllerPostClick(ctrl, 67, 26);
    // MaaControllerWait(ctrl, act);
    // auto act = MaaControllerPostPressKey(ctrl, 0);
    // MaaControllerWait(ctrl, act);

    // MaaControllerDestroy(ctrl);
}
