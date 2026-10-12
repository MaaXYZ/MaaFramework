#include "BatchOcrRepro.h"

#include <cstring>
#include <string>

#include "MaaFramework/MaaAPI.h"

// 回归用例：MaaXYZ/MaaFramework#1507 / #1448。
// batch OCR 曾以所有节点 ROI 的并集做一次 det，而 det 预处理按整张输入图的最大边计算缩放，
// 并集越过 max_side_len 后小 ROI 的文字被连带缩小，det 框变小/碎片化/score 骤降。
// 本用例取自 #1448 的真实截图与管线：小 ROI 的 SelectArtsBattleNormal 与全屏 ROI 的
// ClickScreen 同列表触发 batch，threshold 0.9 下旧实现 score 降至 ~0.81 而识别失败。
// 修复后 batch 分簇保证各节点 det 输入与单独识别同尺度，断言两条路径结果完全一致。

namespace {

bool get_ocr_reco_box(MaaTasker* tasker, MaaTaskId task_id, const std::string& node_name, MaaRect& box)
{
    MaaStringBuffer* task_entry = MaaStringBufferCreate();
    MaaNodeId node_id_list[16] { };
    MaaSize node_id_list_size = std::size(node_id_list);
    MaaStatus status = MaaStatus_Failed;
    bool ret = MaaTaskerGetTaskDetail(tasker, task_id, task_entry, node_id_list, &node_id_list_size, &status)
               && status == MaaStatus_Succeeded;
    MaaStringBufferDestroy(task_entry);
    if (!ret) {
        return false;
    }

    for (MaaSize i = 0; i < node_id_list_size; ++i) {
        MaaStringBuffer* node_name_buf = MaaStringBufferCreate();
        MaaRecoId reco_id = 0;
        MaaActId action_id = 0;
        MaaBool completed = false;
        bool node_ret = MaaTaskerGetNodeDetail(tasker, node_id_list[i], node_name_buf, &reco_id, &action_id, &completed);
        std::string name = node_ret ? MaaStringBufferGet(node_name_buf) : "";
        MaaStringBufferDestroy(node_name_buf);
        if (!node_ret || name != node_name) {
            continue;
        }

        MaaStringBuffer* reco_node_name = MaaStringBufferCreate();
        MaaStringBuffer* algorithm = MaaStringBufferCreate();
        MaaStringBuffer* detail_json = MaaStringBufferCreate();
        MaaImageBuffer* raw = MaaImageBufferCreate();
        MaaImageListBuffer* draws = MaaImageListBufferCreate();
        MaaBool hit = false;
        ret = MaaTaskerGetRecognitionDetail(tasker, reco_id, reco_node_name, algorithm, &hit, &box, detail_json, raw, draws)
              && hit;
        MaaStringBufferDestroy(reco_node_name);
        MaaStringBufferDestroy(algorithm);
        MaaStringBufferDestroy(detail_json);
        MaaImageBufferDestroy(raw);
        MaaImageListBufferDestroy(draws);
        return ret;
    }

    return false;
}

} // namespace

bool batch_ocr_repro(const std::filesystem::path& testset_dir)
{
    auto case_dir = testset_dir / "BatchOcrRepro";

    auto controller = MaaReplayControllerCreate((case_dir / "MaaRecording.jsonl").string().c_str());
    auto resource = MaaResourceCreate();
    auto ctrl_id = MaaControllerPostConnection(controller);
    auto res_id = MaaResourcePostBundle(resource, (case_dir / "resource").string().c_str());

    MaaControllerWait(controller, ctrl_id);
    MaaResourceWait(resource, res_id);

    auto tasker = MaaTaskerCreate();
    MaaTaskerBindResource(tasker, resource);
    MaaTaskerBindController(tasker, controller);

    bool ret = MaaTaskerInited(tasker);

    MaaTaskId batch_task_id = 0;
    MaaTaskId solo_task_id = 0;
    if (ret) {
        batch_task_id = MaaTaskerPostTask(tasker, "SelectArtsBattleMode", "{}");
        MaaTaskerWait(tasker, batch_task_id);

        solo_task_id = MaaTaskerPostTask(tasker, "SelectArtsSolo", "{}");
        MaaTaskerWait(tasker, solo_task_id);
    }

    MaaRect batch_box { };
    MaaRect solo_box { };
    if (ret) {
        ret = get_ocr_reco_box(tasker, batch_task_id, "SelectArtsBattleNormal", batch_box);
    }
    if (ret) {
        ret = get_ocr_reco_box(tasker, solo_task_id, "SelectArtsSolo", solo_box);
    }
    if (ret) {
        // 修复后 batch 路径的 det 输入与单独识别一致，同一确定性推理下两者结果应完全相同
        ret = std::memcmp(&batch_box, &solo_box, sizeof(MaaRect)) == 0;
    }

    MaaTaskerDestroy(tasker);
    MaaResourceDestroy(resource);
    MaaControllerDestroy(controller);

    return ret;
}
