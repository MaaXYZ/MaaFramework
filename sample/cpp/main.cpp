#include <array>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "MaaFramework/MaaAPI.h"
#include "MaaToolkit/MaaToolkitAPI.h"

MaaController* create_adb_controller();
MaaController* create_win32_controller();
MaaController* create_macos_controller();
MaaBool my_reco(
    MaaContext* context,
    MaaTaskId task_id,
    const char* node_name,
    const char* custom_recognition_name,
    const char* custom_recognition_param,
    const MaaImageBuffer* image,
    const MaaRect* roi,
    void* trans_arg,
    /* out */ MaaRect* out_box,
    /* out */ MaaStringBuffer* out_detail);

int main([[maybe_unused]] int argc, char** argv)
{
    std::string user_path = "./";
    MaaToolkitConfigInitOption(user_path.c_str(), "{}");

    auto controller_handle = create_adb_controller();
    // auto controller_handle = create_win32_controller();
    // auto controller_handle = create_macos_controller();
    auto ctrl_id = MaaControllerPostConnection(controller_handle);

    auto resource_handle = MaaResourceCreate();
    std::string resource_dir = R"(E:\Code\MaaFramework\sample\resource)";
    auto res_id = MaaResourcePostBundle(resource_handle, resource_dir.c_str());

    MaaControllerWait(controller_handle, ctrl_id);
    MaaResourceWait(resource_handle, res_id);

    auto tasker_handle = MaaTaskerCreate();
    MaaTaskerBindResource(tasker_handle, resource_handle);
    MaaTaskerBindController(tasker_handle, controller_handle);

    auto destroy = [&]() {
        MaaTaskerDestroy(tasker_handle);
        MaaResourceDestroy(resource_handle);
        MaaControllerDestroy(controller_handle);
    };

    if (!MaaTaskerInited(tasker_handle)) {
        std::cout << "Failed to init MAA" << std::endl;

        destroy();
        return -1;
    }

    MaaResourceRegisterCustomRecognition(resource_handle, "MyReco", my_reco, nullptr);

    auto task_id = MaaTaskerPostTask(tasker_handle, "MyTask", "{}");
    MaaTaskerWait(tasker_handle, task_id);

    destroy();

    return 0;
}

MaaController* create_adb_controller()
{
    auto list_handle = MaaToolkitAdbDeviceListCreate();
    auto destroy = [&]() {
        MaaToolkitAdbDeviceListDestroy(list_handle);
    };

    MaaToolkitAdbDeviceFind(list_handle);

    size_t size = MaaToolkitAdbDeviceListSize(list_handle);
    if (size == 0) {
        std::cout << "No device found" << std::endl;

        destroy();
        return nullptr;
    }

    const int kIndex = 0; // for demo, we just use the first device
    auto device_handle = MaaToolkitAdbDeviceListAt(list_handle, kIndex);

    std::string agent_path = "share/MaaAgentBinary";
    auto controller_handle = MaaAdbControllerCreate(
        MaaToolkitAdbDeviceGetAdbPath(device_handle),
        MaaToolkitAdbDeviceGetAddress(device_handle),
        MaaToolkitAdbDeviceGetScreencapMethods(device_handle),
        MaaToolkitAdbDeviceGetInputMethods(device_handle),
        MaaToolkitAdbDeviceGetConfig(device_handle),
        agent_path.c_str());

    destroy();

    return controller_handle;
}

MaaController* create_win32_controller()
{
    void* hwnd = nullptr; // It's a HWND, you can find it by yourself without MaaToolkit API

    auto list_handle = MaaToolkitDesktopWindowListCreate();
    auto destroy = [&]() {
        MaaToolkitDesktopWindowListDestroy(list_handle);
    };

    MaaToolkitDesktopWindowFindAll(list_handle);

    size_t size = MaaToolkitDesktopWindowListSize(list_handle);

    if (size == 0) {
        std::cout << "No window found" << std::endl;

        destroy();
        return nullptr;
    }

    for (size_t i = 0; i < size; ++i) {
        auto window_handle = MaaToolkitDesktopWindowListAt(list_handle, i);
        std::string class_name = MaaToolkitDesktopWindowGetClassName(window_handle);
        std::string window_name = MaaToolkitDesktopWindowGetWindowName(window_handle);

        if (window_name.find("二重螺旋") != std::string::npos) {
            hwnd = MaaToolkitDesktopWindowGetHandle(window_handle);
            break;
        }
    }

    // create controller by hwnd
    auto controller_handle = MaaWin32ControllerCreate(
        hwnd,
        MaaWin32ScreencapMethod_DXGI_DesktopDup_Window,
        MaaWin32InputMethod_SendMessage,
        MaaWin32InputMethod_SendMessage);

    destroy();
    return controller_handle;
}

MaaController* create_macos_controller()
{
    // For macOS, we need to find a window by its ID
    // You can get window IDs by running the macOS test program first
    uint32_t window_id = 0; // 0 means desktop, or specify a window ID

    auto controller_handle =
        MaaMacOSControllerCreate(window_id, 0, MaaMacOSScreencapMethod_ScreenCaptureKit, MaaMacOSInputMethod_GlobalEvent);

    return controller_handle;
}

// @ MaaCustomRecognitionCallback
MaaBool my_reco(
    MaaContext* context,
    MaaTaskId task_id,
    const char* node_name,
    const char* custom_recognition_name,
    const char* custom_recognition_param,
    const MaaImageBuffer* image,
    const MaaRect* roi,
    void* trans_arg,
    /* out */ MaaRect* out_box,
    /* out */ MaaStringBuffer* out_detail)
{
    /* Get image */

    // Approach 1
    uint8_t* png_data = MaaImageBufferGetEncoded(image);
    size_t png_size = MaaImageBufferGetEncodedSize(image);
    // cv::Mat im = cv::imdecode({ png_data, png_size }, cv::IMREAD_COLOR);

    // Approach 2
    // This approach is more efficient, but may be difficult for some languages.
    // I recommend you to use approach 2 if you can.
    void* raw_data = MaaImageBufferGetRawData(image);
    int32_t width = MaaImageBufferWidth(image);
    int32_t height = MaaImageBufferHeight(image);
    int32_t type = MaaImageBufferType(image);
    // cv::Mat im(height, width, type, raw_data);

    // And do your computer vision...
    // Or you MaaContext API to run some recognition
    MaaRecoId reco_id = MaaContextRunRecognition(context, "MySecondReco", "{}", image);
    auto tasker_handle = MaaContextGetTasker(context);
    // MaaTaskerGetRecognitionDetail(tasker_handle, reco_id, /* ... */);

    /* Output recognition result */

    // Step 1: output box
    std::array<int, 4> my_box { 0 }; // your result
    MaaRectSet(out_box, my_box[0], my_box[1], my_box[2], my_box[3]);

    // Step 2: output anything you want
    MaaStringBufferSet(
        out_detail,
        "Balabala, this string will be used by MaaCustomActionCallback and "
        "MaaQueryRecognitionDetail. "
        "And for compatibility, I recommend you use json.");

    // Finally, if this task is hit and you want to execute the action and next of this task,
    // don't forget to return true!
    return true;
}
