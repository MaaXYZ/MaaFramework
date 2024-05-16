#include <exception>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "MaaPP/MaaPP.hpp"

// for demo, we disable some warnings
#ifdef _MSC_VER
#pragma warning(disable: 4100) // unreferenced formal parameter
#pragma warning(disable: 4189) // local variable is initialized but not referenced
#elif defined(__clang__)
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-variable"
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

using namespace maa;

coro::Promise<ControllerHandle> create_adb_controller();
coro::Promise<ControllerHandle> create_win32_controller();
void register_my_recognizer_by_ffi(InstanceHandle maa_handle);
void register_my_action_by_exec_agent(InstanceHandle maa_handle);

coro::Promise<int> async_main()
{
    std::string user_path = "./";
    init(user_path);

    auto controller = co_await create_adb_controller();
    // auto controller = co_await create_win32_controller();
    auto ctrl_conn_action = controller->post_connect();

    auto resource = Resource::make();
    std::string resource_dir = "my_resource";
    auto res_load_action = resource->post_path(resource_dir);

    co_await ctrl_conn_action->wait();
    co_await res_load_action->wait();
    // or explicit wait all of them concurrently
    // auto [conn_status, load_status] = co_await coro::all(ctrl_conn_action->wait(),
    // res_load_action->wait());

    auto instance = Instance::make();
    instance->bind(controller);
    instance->bind(resource);

    // no need to destroy them

    if (!instance->inited()) {
        std::cout << "Failed to init MAA" << std::endl;
        co_return -1;
    }

    register_my_recognizer_by_ffi(instance);
    register_my_action_by_exec_agent(instance);

    auto action = instance->post_task("MyTask");
    co_await action->wait();

    co_return 0;
}

int main([[maybe_unused]] int argc, char** argv)
{
    coro::EventLoop ev;

    ev.stop_after(async_main());

    return ev.exec();
}

coro::Promise<ControllerHandle> create_adb_controller()
{
    auto devices = co_await AdbDeviceFinder::find();
    if (!devices || devices->size() == 0) {
        std::cout << "No device found" << std::endl;
        co_return nullptr;
    }

    const int kIndex = 0; // for demo, we just use the first device
    std::string agent_path = "share/MaaAgentBinary";
    co_return Controller::make((*devices)[kIndex], agent_path);
}

coro::Promise<ControllerHandle> create_win32_controller()
{
    auto hwnd = Win32DeviceFinder::cursor();
    auto type = Win32Type()
                    .set_touch(MaaWin32ControllerType_Touch_SendMessage)
                    .set_screencap(MaaWin32ControllerType_Screencap_GDI);
    co_return Controller::make(Controller::win32_controller_tag {}, hwnd, type);
}

coro::Promise<AnalyzeResult> my_analyze(
    SyncContextHandle sync_context,
    ImageHandle image,
    std::string_view task_name,
    std::string_view custom_recognition_param)
{
    /* Get image */

    // Approach 1
    auto png_data = image->encoded();
    // cv::Mat im = cv::imdecode({ png_data.data(), png_data.size() }, cv::IMREAD_COLOR);

    std::ignore = png_data;

    // Approach 2 not supported yet

    // And do your computer vision...

    /* Output recognition result */

    AnalyzeResult result;

    // Step 1: output box
    result.rec_box = { 0, 0, 0, 0 }; // your result

    // Step 2: output anything you want
    result.rec_detail =
        "Balabala, this string will be used by MaaCustomActionAPI and MaaQueryRecognitionDetail. "
        "And for compatibility, I recommend you use json.";

    // Finally, if this task is hit and you want to execute the action and next of this task,
    // don't forget to return true!
    result.result = true;

    co_return result;
}

void register_my_recognizer_by_ffi(InstanceHandle maa_handle)
{
    maa_handle->bind("MyRec", CustomRecognizer::make(my_analyze));
}

void register_my_action_by_exec_agent(InstanceHandle maa_handle)
{
    maa_handle->bind_action_executor(
        "MyAct",
        "Python.exe",
        { "sample\\python\\exec_agent\\my_action.py" });
}
