#include "TaskDataMgr.h"

#include "Resource/ResourceMgr.h"
#include "Utils/ImageIo.h"
#include "Utils/Logger.h"

MAA_TASK_NS_BEGIN

TaskDataMgr::TaskDataMgr(InstanceInternalAPI* inst) : inst_(inst) {}

const MAA_RES_NS::TaskData& TaskDataMgr::get_task_data(const std::string& task_name)
{
    auto diff_it = diff_tasks_.find(task_name);
    if (diff_it != diff_tasks_.end()) {
        return diff_it->second;
    }

    if (!resource()) {
        LogError << "Resource not binded";
        static MAA_RES_NS::TaskData empty;
        return empty;
    }

    auto& data_mgr = resource()->pipeline_res();
    return data_mgr.get_task_data(task_name);
}

bool TaskDataMgr::set_param(const json::value& param)
{
    LogFunc << VAR(param);

    bool ret = true;

    auto diff_opt = param.find<json::object>("diff_task");
    if (diff_opt) {
        ret &= set_diff_task(*diff_opt);
    }

    return ret;
}

bool TaskDataMgr::set_diff_task(const json::value& input)
{
    LogFunc << VAR(input);

    if (!resource()) {
        LogError << "Resource not binded";
        return false;
    }

    MAA_RES_NS::PipelineResMgr::TaskDataMap task_data_map;
    auto& raw_data_map = resource()->pipeline_res().get_task_data_map();
    bool parsed = MAA_RES_NS::PipelineResMgr::parse_config(input, task_data_map, raw_data_map);
    if (!parsed) {
        LogError << "Parse json failed";
        return false;
    }

    bool loaded = check_and_load_template_images(task_data_map);
    if (!loaded) {
        LogError << "Load template images failed";
        return false;
    }

    task_data_map.merge(std::move(diff_tasks_));
    diff_tasks_ = std::move(task_data_map);
    return true;
}

bool TaskDataMgr::check_and_load_template_images(TaskDataMap& map)
{
    if (!resource()) {
        LogError << "Resource not binded";
        return false;
    }

    auto& data_mgr = resource()->pipeline_res();

    for (auto& [name, task_data] : map) {
        if (task_data.rec_type != MAA_RES_NS::Recognition::Type::TemplateMatch) {
            continue;
        }
        auto& task_param = std::get<MAA_VISION_NS::TemplMatchingParam>(task_data.rec_param);

        const auto& raw_task = data_mgr.get_task_data(name);

        bool need_load = false;
        if (raw_task.rec_type != MAA_RES_NS::Recognition::Type::TemplateMatch) {
            need_load = true;
        }
        else {
            auto& raw_param = std::get<MAA_VISION_NS::TemplMatchingParam>(raw_task.rec_param);
            if (task_param.template_paths != raw_param.template_paths) {
                need_load = true;
            }
            else {
                task_param.template_images = raw_param.template_images;
                need_load = false;
            }
        }
        if (!need_load) {
            continue;
        }

        for (const auto& path : task_param.template_paths) {
            // TODO: 现在这里只支持绝对路径了，应该要把 TemplateResMgr 里 roots 那套查找方法也弄过来
            cv::Mat templ = imread(path);
            if (templ.empty()) {
                LogError << "Load template failed" << VAR(name) << VAR(path);
                return false;
            }
            task_param.template_images.emplace_back(std::move(templ));
        }
    }

    return true;
}

MAA_TASK_NS_END
