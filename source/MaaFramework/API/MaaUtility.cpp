#include "MaaFramework/MaaAPI.h"

#include "Buffer/ImageBuffer.hpp"
#include "Global/GlobalOptionMgr.h"
#include "Utils/Logger.h"
#include "Utils/NoWarningCVMat.hpp"

const char* MaaVersion()
{
#pragma message("MaaFramework MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaBool MaaSetGlobalOption(MaaGlobalOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    return MAA_NS::GlobalOptionMgr::get_instance().set_option(key, value, val_size);
}

//MaaBool MaaQueryRecognitionDetail(
//    MaaRecoId reco_id,
//    /* out */ MaaStringBuffer* name,
//    /* out */ MaaBool* hit,
//    /* out */ MaaRect* hit_box,
//    /* out */ MaaStringBuffer* detail_json,
//    /* out */ MaaImageBuffer* raw,
//    /* out */ MaaImageListBuffer* draws)
//{
//    bool mhit = false;
//    std::string mname;
//    cv::Rect mbox {};
//    std::string mdetail;
//    cv::Mat mraw;
//    std::vector<cv::Mat> mdraws;
//
//    bool mret =
//        MAA_TASK_NS::Recognizer::query_detail(reco_id, mname, mhit, mbox, mdetail, mraw, mdraws);
//
//    if (!mret) {
//        LogError << "failed to query reco result" << VAR(reco_id);
//        return false;
//    }
//
//    if (name) {
//        name->set(std::move(mname));
//    }
//    if (hit) {
//        *hit = mhit;
//    }
//    if (hit_box) {
//        hit_box->x = mbox.x;
//        hit_box->y = mbox.y;
//        hit_box->width = mbox.width;
//        hit_box->height = mbox.height;
//    }
//    if (detail_json) {
//        detail_json->set(std::move(mdetail));
//    }
//    if (raw) {
//        raw->set(std::move(mraw));
//    }
//    if (draws) {
//        for (auto& d : mdraws) {
//            draws->append(MAA_NS::ImageBuffer(std::move(d)));
//        }
//    }
//    return true;
//}
//
//MaaBool MaaQueryNodeDetail(
//    MaaNodeId node_id,
//    /* out */ MaaStringBuffer* name,
//    /* out */ MaaRecoId* reco_id,
//    /* out */ MaaBool* run_completed)
//{
//    std::string mname;
//    MaaRecoId mreco_id = 0;
//    bool mcompleted = false;
//
//    bool mret = MAA_TASK_NS::PipelineTask::query_node_detail(node_id, mname, mreco_id, mcompleted);
//    if (!mret) {
//        LogError << "failed to query running detail" << VAR(node_id);
//        return false;
//    }
//
//    if (name) {
//        name->set(std::move(mname));
//    }
//    if (reco_id) {
//        *reco_id = mreco_id;
//    }
//    if (run_completed) {
//        *run_completed = mcompleted;
//    }
//    return true;
//}
//
//MaaBool MaaQueryTaskDetail(
//    MaaTaskId task_id,
//    /* out */ MaaStringBuffer* entry,
//    /* out */ MaaNodeId* node_id_list,
//    /* in & out */ MaaSize* node_id_list_size)
//{
//    std::string mentry;
//    std::vector<MaaNodeId> node_id_vec;
//    bool ret = MAA_TASK_NS::PipelineTask::query_task_detail(task_id, mentry, node_id_vec);
//    if (!ret) {
//        LogError << "failed to query task detail" << VAR(task_id);
//        return false;
//    }
//
//    if (entry) {
//        entry->set(std::move(mentry));
//    }
//
//    if (node_id_list_size && *node_id_list_size == 0) {
//        *node_id_list_size = node_id_vec.size();
//    }
//    else if (node_id_list && node_id_list_size) {
//        size_t size = std::min(node_id_vec.size(), static_cast<size_t>(*node_id_list_size));
//        memcpy(node_id_list, node_id_vec.data(), size * sizeof(MaaNodeId));
//        *node_id_list_size = size;
//    }
//    else {
//        LogError << "failed to query task detail" << VAR(task_id) << VAR(node_id_list)
//                 << VAR(node_id_list_size);
//        return false;
//    }
//
//    return true;
//}
