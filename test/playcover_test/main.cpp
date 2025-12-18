#include <filesystem>
#include <iostream>
#include <string>

#include "MaaFramework/MaaAPI.h"

int main(int argc, char** argv)
{
    std::string address = "127.0.0.1:1717";
    std::string uuid = "com.hypergryph.arknights";

    if (argc >= 2) {
        address = argv[1];
    }
    if (argc >= 3) {
        uuid = argv[2];
    }

    auto cur_dir = std::filesystem::path(argv[0]).parent_path();
    std::string logging_dir = (cur_dir / "debug").string();
    MaaGlobalSetOption(MaaGlobalOption_LogDir, static_cast<void*>(logging_dir.data()), logging_dir.size());

    MaaLoggingLevel lv = MaaLoggingLevel_Info;
    MaaGlobalSetOption(MaaGlobalOption_StdoutLevel, &lv, sizeof(lv));

    std::cout << "Creating PlayCover controller..." << std::endl;
    std::cout << "Address: " << address << std::endl;
    std::cout << "UUID: " << uuid << std::endl;

    auto controller = MaaPlayCoverControllerCreate(address.c_str(), uuid.c_str());
    if (!controller) {
        std::cerr << "Failed to create PlayCover controller" << std::endl;
        return -1;
    }

    std::cout << "Connecting to PlayCover..." << std::endl;
    auto conn_id = MaaControllerPostConnection(controller);
    if (conn_id == MaaInvalidId) {
        std::cerr << "Failed to queue connection request" << std::endl;
        MaaControllerDestroy(controller);
        return -1;
    }

    auto status = MaaControllerWait(controller, conn_id);
    if (status != MaaStatus_Succeeded) {
        std::cerr << "Failed to connect to PlayCover, status: " << status << std::endl;
        MaaControllerDestroy(controller);
        return -1;
    }

    std::cout << "Connected successfully!" << std::endl;

    auto resource = MaaResourceCreate();
    if (!resource) {
        std::cerr << "Failed to create resource" << std::endl;
        MaaControllerDestroy(controller);
        return -1;
    }

    auto resource_dir = cur_dir / "resource";
    if (!std::filesystem::exists(resource_dir)) {
        std::filesystem::create_directories(resource_dir / "pipeline");
    }

    std::string resource_path = resource_dir.string();
    auto res_id = MaaResourcePostBundle(resource, resource_path.c_str());
    if (res_id == MaaInvalidId) {
        std::cerr << "Failed to queue resource bundle load" << std::endl;
        MaaResourceDestroy(resource);
        MaaControllerDestroy(controller);
        return -1;
    }

    auto res_status = MaaResourceWait(resource, res_id);
    if (res_status != MaaStatus_Succeeded) {
        std::cerr << "Resource bundle load failed, status: " << res_status << std::endl;
        MaaResourceDestroy(resource);
        MaaControllerDestroy(controller);
        return -1;
    }

    auto tasker = MaaTaskerCreate();
    if (!tasker) {
        std::cerr << "Failed to create tasker" << std::endl;
        MaaResourceDestroy(resource);
        MaaControllerDestroy(controller);
        return -1;
    }

    if (!MaaTaskerBindResource(tasker, resource) || !MaaTaskerBindController(tasker, controller)) {
        std::cerr << "Failed to bind tasker with resource/controller" << std::endl;
        MaaTaskerDestroy(tasker);
        MaaResourceDestroy(resource);
        MaaControllerDestroy(controller);
        return -1;
    }

    if (!MaaTaskerInited(tasker)) {
        std::cerr << "Failed to init tasker" << std::endl;
        MaaTaskerDestroy(tasker);
        MaaResourceDestroy(resource);
        MaaControllerDestroy(controller);
        return -1;
    }

    std::cout << "Taking screenshot..." << std::endl;
    auto screencap_id = MaaControllerPostScreencap(controller);
    if (screencap_id == MaaInvalidId) {
        std::cerr << "Failed to queue screencap request" << std::endl;
        MaaTaskerDestroy(tasker);
        MaaResourceDestroy(resource);
        MaaControllerDestroy(controller);
        return -1;
    }

    auto screencap_status = MaaControllerWait(controller, screencap_id);
    if (screencap_status != MaaStatus_Succeeded) {
        std::cerr << "Screencap request failed, status: " << screencap_status << std::endl;
        MaaTaskerDestroy(tasker);
        MaaResourceDestroy(resource);
        MaaControllerDestroy(controller);
        return -1;
    }

    auto image_buffer = MaaImageBufferCreate();
    if (!MaaControllerCachedImage(controller, image_buffer)) {
        std::cerr << "Failed to get cached image" << std::endl;
        MaaImageBufferDestroy(image_buffer);
        MaaTaskerDestroy(tasker);
        MaaResourceDestroy(resource);
        MaaControllerDestroy(controller);
        return -1;
    }

    int32_t width = MaaImageBufferWidth(image_buffer);
    int32_t height = MaaImageBufferHeight(image_buffer);
    std::cout << "Screenshot size: " << width << "x" << height << std::endl;

    std::cout << "Running OCR recognition..." << std::endl;

    std::string ocr_param = R"({
        "recognition": "OCR",
        "roi": [0, 0, 0, 0],
        "only_rec": false
    })";

    auto task_id = MaaTaskerPostRecognition(tasker, "OCR", ocr_param.c_str(), image_buffer);
    if (task_id == MaaInvalidId) {
        std::cerr << "Failed to queue OCR recognition task" << std::endl;
        MaaImageBufferDestroy(image_buffer);
        MaaTaskerDestroy(tasker);
        MaaResourceDestroy(resource);
        MaaControllerDestroy(controller);
        return -1;
    }

    auto task_status = MaaTaskerWait(tasker, task_id);

    if (task_status != MaaStatus_Succeeded) {
        std::cerr << "OCR task failed, status: " << task_status << std::endl;
        MaaImageBufferDestroy(image_buffer);
        MaaTaskerDestroy(tasker);
        MaaResourceDestroy(resource);
        MaaControllerDestroy(controller);
        return -1;
    }

    auto entry_buffer = MaaStringBufferCreate();
    MaaNodeId node_id_list[16] = { 0 };
    MaaSize node_id_list_size = 16;
    MaaStatus task_detail_status = MaaStatus_Invalid;

    if (!MaaTaskerGetTaskDetail(tasker, task_id, entry_buffer, node_id_list, &node_id_list_size, &task_detail_status)) {
        std::cerr << "Failed to get task detail" << std::endl;
        MaaStringBufferDestroy(entry_buffer);
        MaaImageBufferDestroy(image_buffer);
        MaaTaskerDestroy(tasker);
        MaaResourceDestroy(resource);
        MaaControllerDestroy(controller);
        return -1;
    }

    std::cout << "\n=== OCR Result ===" << std::endl;
    std::cout << "Task entry: " << MaaStringBufferGet(entry_buffer) << std::endl;
    std::cout << "Node count: " << node_id_list_size << std::endl;

    for (MaaSize i = 0; i < node_id_list_size; ++i) {
        auto node_name_buffer = MaaStringBufferCreate();
        MaaRecoId reco_id = 0;
        MaaActId action_id = 0;
        MaaBool completed = false;

        if (MaaTaskerGetNodeDetail(tasker, node_id_list[i], node_name_buffer, &reco_id, &action_id, &completed)) {
            auto detail_buffer = MaaStringBufferCreate();
            auto algo_buffer = MaaStringBufferCreate();
            MaaBool hit = false;
            MaaRect box = { 0, 0, 0, 0 };

            if (MaaTaskerGetRecognitionDetail(tasker, reco_id, nullptr, algo_buffer, &hit, &box, detail_buffer, nullptr, nullptr)) {
                const char* detail = MaaStringBufferGet(detail_buffer);
                std::cout << "Algorithm: " << MaaStringBufferGet(algo_buffer) << std::endl;
                std::cout << "Hit: " << (hit ? "true" : "false") << std::endl;
                std::cout << "Detail: " << (detail ? detail : "null") << std::endl;
            }

            MaaStringBufferDestroy(algo_buffer);
            MaaStringBufferDestroy(detail_buffer);
        }

        MaaStringBufferDestroy(node_name_buffer);
    }

    std::cout << "==================\n" << std::endl;

    MaaStringBufferDestroy(entry_buffer);
    MaaImageBufferDestroy(image_buffer);
    MaaTaskerDestroy(tasker);
    MaaResourceDestroy(resource);
    MaaControllerDestroy(controller);

    std::cout << "Test completed." << std::endl;
    return 0;
}
