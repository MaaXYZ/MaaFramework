#include "MaaFramework/MaaAPI.h"

#include "Buffer/ImageBuffer.hpp"
#include "Global/GlobalOptionMgr.h"
#include "Task/PipelineTask.h"
#include "Task/Recognizer.h"
#include "Utils/Logger.h"
#include "Utils/NoWarningCVMat.hpp"

MaaStringView MaaVersion()
{
#pragma message("MaaFramework MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaBool MaaSetGlobalOption(MaaGlobalOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    return MAA_NS::GlobalOptionMgr::get_instance().set_option(key, value, val_size);
}

MaaBool MaaQueryRecognitionDetail(
    MaaRecoId reco_id,
    /* out */ MaaBool* hit,
    /* out */ MaaRectHandle hit_box,
    /* out */ MaaStringBufferHandle detail_json,
    /* out */ MaaImageListBufferHandle draws)
{
    bool mhit = false;
    cv::Rect mbox {};
    std::string mdetail;
    std::vector<cv::Mat> mdraws;

    bool mret = MAA_TASK_NS::Recognizer::query_detail(reco_id, mhit, mbox, mdetail, mdraws);

    if (!mret) {
        LogError << "failed to query reco result" << VAR(reco_id);
        return false;
    }

    if (hit) {
        *hit = mhit;
    }
    if (hit_box) {
        hit_box->x = mbox.x;
        hit_box->y = mbox.y;
        hit_box->width = mbox.width;
        hit_box->height = mbox.height;
    }
    if (detail_json) {
        detail_json->set(std::move(mdetail));
    }
    if (draws) {
        for (auto& d : mdraws) {
            draws->append(MAA_NS::ImageBuffer(std::move(d)));
        }
    }
    return true;
}

MaaBool MaaQueryRunningDetail(
    MaaRunningId run_id,
    /*out*/ MaaRecoId* reco_id,
    /*out*/ MaaBool* successful)
{
    MaaRecoId mreco_id = 0;
    bool msuccessful = false;

    bool mret = MAA_TASK_NS::PipelineTask::query_running_detail(run_id, mreco_id, msuccessful);
    if (!mret) {
        LogError << "failed to query running detail" << VAR(run_id);
        return false;
    }

    if (reco_id) {
        *reco_id = mreco_id;
    }
    if (successful) {
        *successful = msuccessful;
    }
    return true;
}

MaaBool MaaQueryTaskDetail(MaaTaskId task_id, MaaRunningId* run_id_list, MaaSize* run_id_list_size)
{
    std::vector<MaaRunningId> run_id_vec;
    bool ret = MAA_TASK_NS::PipelineTask::query_task_detail(task_id, run_id_vec);
    if (!ret) {
        LogError << "failed to query task detail" << VAR(task_id);
        return false;
    }

    if (run_id_list_size && *run_id_list_size == 0) {
        *run_id_list_size = run_id_vec.size();
        return true;
    }
    else if (run_id_list && run_id_list_size) {
        memcpy(run_id_list, run_id_vec.data(), std::min(run_id_vec.size(), *run_id_list_size));
        return true;
    }
    else {
        LogError << "failed to query task detail" << VAR(task_id) << VAR(run_id_list)
                 << VAR(run_id_list_size);
        return false;
    }
}
