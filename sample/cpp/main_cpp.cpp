#include <iostream>

#include "MaaPP/MaaPP.h"

std::shared_ptr<maapp::Controller> create_adb_controller();

int main()
{
    std::string user_path = "./";
    MaaToolkitConfigInitOption(user_path.c_str(), "{}");

    auto controller = create_adb_controller();
    // auto controller = create_win32_controller();
    auto conn_task = controller->post_connection();

    auto resource = std::make_shared<maapp::Resource>();
    std::string resource_dir = R"(E:\Code\MaaFramework\sample\resource)";
    auto load_task = resource->post_bundle(resource_dir);

    conn_task.wait();
    load_task.wait();

    auto tasker = std::make_shared<maapp::Tasker>();
    tasker->bind(resource);
    tasker->bind(controller);

    if (!tasker->inited()) {
        std::cout << "Failed to init MAA" << std::endl;

        return -1;
    }

    resource->register_custom_recognition("MyReco", [](maapp::CustomRecognitionRequest req) -> maapp::CustomRecognitionResponse {
        [[maybe_unused]] const auto& png_data = req.image;

        [[maybe_unused]] auto reco = req.context.run_recognition("MySecondReco", "{}", png_data);
        [[maybe_unused]] auto tasker = req.context.tasker();

        return {
            true,
            { 0, 0, 0, 0 },
            "Balabala, this string will be used by MaaCustomActionCallback and "
            "MaaQueryRecognitionDetail. "
            "And for compatibility, I recommend you use json.",
        };
    });

    auto task = tasker->post_task("MyTask", "{}");
    task.wait();

    return 0;
}

std::shared_ptr<maapp::Controller> create_adb_controller()
{
    // TODO
    return nullptr;
}
