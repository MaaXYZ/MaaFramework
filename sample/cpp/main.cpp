#include "MaaFramework/MaaAPI.h"

#include <array>
#include <filesystem>
#include <iostream>
#include <string>

#include "MaaFramework/MaaCustomRecognizer.h"
#include "MaaToolKit/MaaToolKitAPI.h"

#ifdef _WIN32
// for demo, we disable some warnings
#pragma warning(disable : 4100) // unreferenced formal parameter
#pragma warning(disable : 4189) // local variable is initialized but not referenced
#endif

void register_my_recognizer(MaaInstanceHandle maa_handle);

int main([[maybe_unused]] int argc, char** argv)
{
    MaaToolKitInit();
    auto device_size = MaaToolKitFindDevice();
    if (device_size == 0) {
        std::cout << "No device found" << std::endl;
        return 0;
    }

    const int kIndex = 0; // for demo, we just use the first device
    auto controller_handle = MaaAdbControllerCreate(
        MaaToolKitGetDeviceAdbPath(kIndex), MaaToolKitGetDeviceAdbSerial(kIndex),
        MaaToolKitGetDeviceAdbControllerType(kIndex), MaaToolKitGetDeviceAdbConfig(kIndex), nullptr, nullptr);
    auto ctrl_id = MaaControllerPostConnection(controller_handle);
    MaaControllerWait(controller_handle, ctrl_id);

    auto resource_handle = MaaResourceCreate(nullptr, nullptr);
    auto resource_dir = std::filesystem::path(argv[0]).parent_path() / "resource";
    auto res_id = MaaResourcePostPath(resource_handle, resource_dir.string().c_str());
    MaaResourceWait(resource_handle, res_id);

    auto maa_handle = MaaCreate(nullptr, nullptr);
    MaaBindResource(maa_handle, resource_handle);
    MaaBindController(maa_handle, controller_handle);

    if (!MaaInited(maa_handle)) {
        std::cout << "Failed to init MAA" << std::endl;

        MaaDestroy(maa_handle);
        MaaResourceDestroy(resource_handle);
        MaaControllerDestroy(controller_handle);
        MaaToolKitUninit();
        return 0;
    }

    register_my_recognizer(maa_handle);

    auto task_id = MaaPostTask(maa_handle, "MyTask", MaaTaskParam_Empty);
    MaaWaitTask(maa_handle, task_id);

    MaaDestroy(maa_handle);
    MaaResourceDestroy(resource_handle);
    MaaControllerDestroy(controller_handle);
    MaaToolKitUninit();

    return 0;
}

MaaBool my_analyze(MaaSyncContextHandle sync_context, const MaaImageBufferHandle image, MaaStringView task_name,
                   MaaStringView custom_recognition_param,
                   /*out*/ MaaRectHandle out_box,
                   /*out*/ MaaStringBufferHandle detail_buff)
{
    /* Get image */

    // Approach 1
    uint8_t* png_data = MaaGetImageEncoded(image);
    size_t png_size = MaaGetImageEncodedSize(image);
    // cv::Mat im = cv::imdecode({ png_data, png_size }, cv::IMREAD_COLOR);

    // Approach 2
    // This approach is more efficient, but may be difficult for some languages.
    // I recommend you to use approach 2 if you can.
    void* raw_data = MaaGetImageRawData(image);
    int32_t width = MaaGetImageWidth(image);
    int32_t height = MaaGetImageHeight(image);
    int32_t type = MaaGetImageType(image);
    // cv::Mat im(height, width, type, raw_data);

    // And do your computer vision...

    /* Output recognition result */

    // Step 1: output out_box
    std::array<int, 4> my_box { 0 }; // your result
    out_box->x = my_box[0];
    out_box->y = my_box[1];
    out_box->width = my_box[2];
    out_box->height = my_box[3];

    // Step 2: output anything you want
    MaaSetString(detail_buff,
                 "Balabala, this string will be used by MaaCustomActionAPI and MaaSyncContextGetTaskResult. "
                 "And for compatibility, I recommend you use json.");

    // Finally, don't forget to return a true!
    return true;
}

MaaCustomRecognizerAPI my_recognizer;

void register_my_recognizer(MaaInstanceHandle maa_handle)
{
    my_recognizer.analyze = my_analyze;
    MaaRegisterCustomRecognizer(maa_handle, "MyRec", &my_recognizer);
}
