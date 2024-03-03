#include "InstanceStatus.h"

#include "Utils/Logger.h"

MAA_NS_BEGIN

const cv::Rect& InstanceStatus::get_rec_box(const std::string& task) const
{
    auto it = rec_box_map_.find(task);
    if (it == rec_box_map_.end()) {
        static cv::Rect empty {};
        return empty;
    }
    return it->second;
}

void InstanceStatus::set_rec_box(std::string task, cv::Rect rec)
{
    rec_box_map_.insert_or_assign(std::move(task), std::move(rec));
}

void InstanceStatus::clear_rec_box()
{
    LogTrace;

    rec_box_map_.clear();
}

const json::value& InstanceStatus::get_rec_detail(const std::string& task) const
{
    auto it = rec_detail_map_.find(task);
    if (it == rec_detail_map_.end()) {
        static json::value empty;
        return empty;
    }
    return it->second;
}

void InstanceStatus::set_rec_detail(std::string task, json::value detail)
{
    rec_detail_map_.insert_or_assign(std::move(task), std::move(detail));
}

void InstanceStatus::clear_rec_detail()
{
    LogTrace;

    rec_detail_map_.clear();
}

const json::value& InstanceStatus::get_task_result(const std::string& task) const
{
    auto it = task_result_map_.find(task);
    if (it == task_result_map_.end()) {
        static json::value empty;
        return empty;
    }
    return it->second;
}

void InstanceStatus::set_task_result(std::string task, json::value result)
{
    task_result_map_.insert_or_assign(std::move(task), std::move(result));
}

void InstanceStatus::clear_task_result()
{
    LogTrace;

    task_result_map_.clear();
}

uint64_t InstanceStatus::get_run_times(const std::string& task) const
{
    auto it = run_times_map_.find(task);
    if (it == run_times_map_.end()) {
        return 0ULL;
    }
    return it->second;
}

void InstanceStatus::increase_run_times(const std::string& task, int times)
{
    run_times_map_[task] += times;
}

void InstanceStatus::clear_run_times()
{
    LogTrace;

    run_times_map_.clear();
}

std::optional<std::any> InstanceStatus::get_ocr_cache(const cv::Mat& image) const
{
    return ocr_cache_.get_cache(image);
}

void InstanceStatus::set_ocr_cache(cv::Mat image, std::any result)
{
    ocr_cache_.set_cache(std::move(image), std::move(result));
}

void InstanceStatus::clear_ocr_cache()
{
    LogTrace;

    ocr_cache_.clear();
}

bool InstanceStatus::cv_mat_equal(const cv::Mat& lhs, const cv::Mat& rhs)
{
    // treat two empty mat as identical as well
    if (lhs.empty() && rhs.empty()) {
        return true;
    }
    // if dimensionality of two mat is not identical, these two mat is not identical
    if (lhs.cols != rhs.cols || lhs.rows != rhs.rows || lhs.dims != rhs.dims) {
        return false;
    }
    cv::Mat diff;
    cv::compare(lhs, rhs, diff, cv::CMP_NE);
    int nz = cv::countNonZero(diff);
    return nz == 0;
}

MAA_NS_END
