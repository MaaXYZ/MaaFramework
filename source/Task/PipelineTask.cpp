#include "PipelineTask.h"

#include "Controller/ControllerMgr.h"
#include "Instance/InstanceStatus.h"
#include "MaaUtils/Logger.hpp"
#include "Resource/ResourceMgr.h"
#include "Task/CustomAction.h"
#include "Utils/ImageIo.hpp"
#include "Vision/Comparator.h"
#include "Vision/CustomRecognizer.h"
#include "Vision/Matcher.h"
#include "Vision/OCRer.h"
#include "Vision/VisionUtils.hpp"

MAA_TASK_NS_BEGIN

PipelineTask::PipelineTask(std::string first_task_name, InstanceInternalAPI* inst)
    : inst_(inst), first_task_name_(std::move(first_task_name))
{}

bool PipelineTask::run()
{
    LogFunc << VAR(first_task_name_);

    if (!resource()) {
        LogError << "Resource not binded";
        return false;
    }

    auto cur_task = get_task_data(first_task_name_);
    cur_task_name_ = cur_task.name;
    std::vector<std::string> next_list = { first_task_name_ };
    std::stack<std::string> breakpoints_stack;
    std::string pre_breakpoint;

    RunningResult ret = RunningResult::Success;
    while (!next_list.empty() && !need_exit()) {
        ret = find_first_and_run(next_list, cur_task.timeout, cur_task);
        cur_task_name_ = cur_task.name;

        switch (ret) {
        case RunningResult::Success:
            next_list = cur_task.next;
            break;
        case RunningResult::Timeout:
            next_list = cur_task.timeout_next;
            break;
        case RunningResult::Runout:
            next_list = cur_task.runout_next;
            break;
        case RunningResult::Interrupted:
            next_list.clear();
            break;
        default:
            break;
        }

        if (cur_task.is_sub) {
            breakpoints_stack.emplace(pre_breakpoint);
            LogInfo << "breakpoints add" << pre_breakpoint;
        }

        if (next_list.empty() && !breakpoints_stack.empty()) {
            std::string top_bp = std::move(breakpoints_stack.top());
            breakpoints_stack.pop();
            pre_breakpoint = top_bp;
            next_list = get_task_data(top_bp).next;
            LogInfo << "breakpoints pop" << VAR(top_bp) << VAR(next_list);
        }
        else {
            pre_breakpoint = cur_task.name;
        }
    }

    return ret == RunningResult::Success;
}

bool PipelineTask::set_param(const json::value& param)
{
    LogFunc << VAR(param);

    bool ret = true;

    auto diff_opt = param.find<json::object>("diff_task");
    if (diff_opt) {
        ret &= set_diff_task(*diff_opt);
    }

    return ret;
}

bool PipelineTask::set_diff_task(const json::value& input)
{
    LogFunc << VAR(input);

    if (!resource()) {
        LogError << "Resource not binded";
        return false;
    }

    MAA_RES_NS::PipelineConfig::TaskDataMap task_data_map;
    auto& raw_data_map = resource()->pipeline_cfg().get_task_data_map();
    bool parsed = MAA_RES_NS::PipelineConfig::parse_json(input, task_data_map, raw_data_map);
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

bool PipelineTask::check_and_load_template_images(TaskDataMap& map)
{
    if (!resource()) {
        LogError << "Resource not binded";
        return false;
    }

    auto& data_mgr = resource()->pipeline_cfg();

    for (auto& [name, task_data] : map) {
        if (task_data.rec_type != MAA_PIPELINE_RES_NS::Recognition::Type::TemplateMatch) {
            continue;
        }
        auto& task_param = std::get<MAA_VISION_NS::TemplMatchingParam>(task_data.rec_param);

        const auto& raw_task = data_mgr.get_task_data(name);

        bool need_load = false;
        if (raw_task.rec_type != MAA_PIPELINE_RES_NS::Recognition::Type::TemplateMatch) {
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

PipelineTask::RunningResult PipelineTask::find_first_and_run(const std::vector<std::string>& list,
                                                             std::chrono::milliseconds find_timeout,
                                                             /*out*/ MAA_PIPELINE_RES_NS::TaskData& found_data)
{
    if (!status()) {
        LogError << "Status not binded";
        return RunningResult::InternalError;
    }
    FoundResult result;

    auto start_time = std::chrono::steady_clock::now();
    while (true) {
        auto find_opt = find_first(list);
        if (find_opt) {
            result = *std::move(find_opt);
            break;
        }

        if (std::chrono::steady_clock::now() - start_time > find_timeout) {
            return RunningResult::Timeout;
        }
        if (need_exit()) {
            return RunningResult::Interrupted;
        }
    }
    if (need_exit()) {
        return RunningResult::Interrupted;
    }
    const std::string& name = result.task_data.name;
    LogInfo << "Task hit:" << name << VAR(result.rec.box);

    uint64_t run_times = status()->get_pipeline_run_times(name);
    if (result.task_data.times_limit <= run_times) {
        LogInfo << "Task runout:" << name;

        found_data = std::move(result.task_data);
        return RunningResult::Runout;
    }

    start_to_act(result);

    status()->increase_pipeline_run_times(name);

    found_data = std::move(result.task_data);
    return RunningResult::Success;
}

std::optional<PipelineTask::FoundResult> PipelineTask::find_first(const std::vector<std::string>& list)
{
    if (!controller()) {
        LogError << "Controller not binded";
        return std::nullopt;
    }

    LogFunc << VAR(cur_task_name_) << VAR(list);

    cv::Mat image = controller()->screencap();

    for (const std::string& name : list) {
        LogTrace << "recognize:" << name;
        const auto& task_data = get_task_data(name);
        auto rec_opt = recognize(image, task_data);
        if (!rec_opt) {
            continue;
        }
        return FoundResult { .rec = *std::move(rec_opt), .task_data = task_data };
    }
    return std::nullopt;
}

std::optional<PipelineTask::RecResult> PipelineTask::recognize(const cv::Mat& image,
                                                               const MAA_PIPELINE_RES_NS::TaskData& task_data)
{
    using namespace MAA_PIPELINE_RES_NS::Recognition;
    using namespace MAA_VISION_NS;

    if (!status()) {
        LogError << "Status not binded";
        return std::nullopt;
    }
    cv::Rect cache {};
    if (task_data.cache) {
        cache = status()->get_pipeline_rec_cache(task_data.name);
    }

    std::optional<PipelineTask::RecResult> result;
    switch (task_data.rec_type) {
    case Type::DirectHit:
        result = direct_hit(image, std::get<DirectHitParam>(task_data.rec_param), cache);
        break;

    case Type::TemplateMatch:
        result = template_match(image, std::get<TemplMatchingParam>(task_data.rec_param), cache);
        break;

    case Type::OCR:
        result = ocr(image, std::get<OcrParam>(task_data.rec_param), cache);
        break;

    case Type::Custom:
        result = custom_recognize(image, std::get<CustomParam>(task_data.rec_param), cache);
        break;

    default:
        LogError << "Unknown type" << VAR(static_cast<int>(task_data.rec_type));
        return std::nullopt;
    }

    if (result) {
        status()->set_pipeline_rec_cache(task_data.name, result->box);
    }
    return result;
}

std::optional<PipelineTask::RecResult> PipelineTask::direct_hit(const cv::Mat& image,
                                                                const MAA_VISION_NS::DirectHitParam& param,
                                                                const cv::Rect& cache)
{
    std::ignore = image;
    std::ignore = cache;

    return RecResult { .box = param.roi.empty() ? cv::Rect() : param.roi.front() };
}

std::optional<PipelineTask::RecResult> PipelineTask::template_match(const cv::Mat& image,
                                                                    const MAA_VISION_NS::TemplMatchingParam& param,
                                                                    const cv::Rect& cache)
{
    using namespace MAA_VISION_NS;

    Matcher matcher(inst_, image);
    matcher.set_param(param);
    matcher.set_cache(cache);

    auto ret = matcher.analyze();
    if (!ret) {
        return std::nullopt;
    }
    return RecResult { .box = ret->box };
}

std::optional<PipelineTask::RecResult> PipelineTask::ocr(const cv::Mat& image, const MAA_VISION_NS::OcrParam& param,
                                                         const cv::Rect& cache)
{
    using namespace MAA_VISION_NS;

    OCRer ocer(inst_, image);
    ocer.set_param(param);
    ocer.set_cache(cache);

    auto ret = ocer.analyze();
    if (!ret) {
        return std::nullopt;
    }
    auto& res = *ret;

    // TODO: sort by required regex.
    // sort_by_required_(res, param.text);

    return RecResult { .box = res.front().box };
}

std::optional<PipelineTask::RecResult> PipelineTask::custom_recognize(const cv::Mat& image,
                                                                      const MAA_VISION_NS::CustomParam& param,
                                                                      const cv::Rect& cache)
{
    using namespace MAA_VISION_NS;

    std::ignore = cache;

    if (!inst_) {
        LogError << "Instance not binded";
        return std::nullopt;
    }

    auto recognizer = inst_->custom_recognizer(param.name);
    if (!recognizer) {
        LogError << "Custom recognizer not found:" << param.name;
        return std::nullopt;
    }
    recognizer->set_image(image);
    recognizer->set_param(param);

    auto ret = recognizer->analyze();
    if (!ret) {
        return std::nullopt;
    }

    return RecResult { .box = ret->box };
}

void PipelineTask::start_to_act(const FoundResult& act)
{
    using namespace MAA_PIPELINE_RES_NS::Action;
    LogFunc << VAR(act.task_data.name);

    wait_freezes(act.task_data.pre_wait_freezes, act.rec.box);
    sleep(act.task_data.pre_delay);

    switch (act.task_data.action_type) {
    // TODO: 这些内部 aciton 也可以作为但一个单独的 InterAction 类，但好像也没啥必要（
    case Type::DoNothing:
        break;
    case Type::Click:
        click(std::get<ClickParam>(act.task_data.action_param), act.rec.box);
        break;
    case Type::Swipe:
        swipe(std::get<SwipeParam>(act.task_data.action_param), act.rec.box);
        break;
    case Type::Key:
        press_key(std::get<KeyParam>(act.task_data.action_param));
        break;
    case Type::StartApp:
        start_app(std::get<AppParam>(act.task_data.action_param));
        break;
    case Type::StopApp:
        stop_app(std::get<AppParam>(act.task_data.action_param));
        break;
    case Type::Custom:
        custom_action(std::get<CustomParam>(act.task_data.action_param), act.rec.box);
        break;
    default:
        LogError << "Unknown action" << VAR(static_cast<int>(act.task_data.action_type));
        break;
    }

    wait_freezes(act.task_data.post_wait_freezes, act.rec.box);
    sleep(act.task_data.post_delay);
}

void PipelineTask::click(const MAA_PIPELINE_RES_NS::Action::ClickParam& param, const cv::Rect& cur_box)
{
    if (!controller()) {
        LogError << "Controller is null";
        return;
    }

    cv::Rect rect = get_target_rect(param.target, cur_box);

    controller()->click(rect);
}

void PipelineTask::swipe(const MAA_PIPELINE_RES_NS::Action::SwipeParam& param, const cv::Rect& cur_box)
{
    if (!controller()) {
        LogError << "Controller is null";
        return;
    }

    cv::Rect begin = get_target_rect(param.begin, cur_box);
    cv::Rect end = get_target_rect(param.end, cur_box);

    controller()->swipe(begin, end, param.duration);
}

void PipelineTask::press_key(const MAA_PIPELINE_RES_NS::Action::KeyParam& param)
{
    if (!controller()) {
        LogError << "Controller is null";
        return;
    }
    for (const auto& key : param.keys) {
        controller()->press_key(key);
    }
}

void PipelineTask::wait_freezes(const MAA_PIPELINE_RES_NS::WaitFreezesParam& param, const cv::Rect& cur_box)
{
    if (param.time <= std::chrono::milliseconds(0)) {
        return;
    }

    if (!controller()) {
        LogError << "Controller is null";
        return;
    }
    using namespace MAA_VISION_NS;

    LogFunc << "Wait freezes:" << VAR(param.time) << VAR(param.threshold) << VAR(param.method);

    cv::Rect target = get_target_rect(param.target, cur_box);

    Comparator comp;
    comp.set_param({
        .roi = { target },
        .threshold = param.threshold,
        .method = param.method,
    });

    cv::Mat pre_image = controller()->screencap();
    auto pre_time = std::chrono::steady_clock::now();

    while (!need_exit()) {
        cv::Mat cur_image = controller()->screencap();
        if (!comp.analyze(pre_image, cur_image)) {
            pre_image = cur_image;
            pre_time = std::chrono::steady_clock::now();
            continue;
        }

        if (duration_since(pre_time) > param.time) {
            break;
        }
    }
}

void PipelineTask::start_app(const MAA_PIPELINE_RES_NS::Action::AppParam& param)
{
    if (!controller()) {
        LogError << "Controller is null";
        return;
    }
    using namespace MAA_VISION_NS;

    controller()->start_app(param.package);
}

void PipelineTask::stop_app(const MAA_PIPELINE_RES_NS::Action::AppParam& param)
{
    if (!controller()) {
        LogError << "Controller is null";
        return;
    }
    using namespace MAA_VISION_NS;

    controller()->stop_app(param.package);
}

void PipelineTask::custom_action(const MAA_PIPELINE_RES_NS::Action::CustomParam& param, const cv::Rect& cur_box)
{
    if (!inst_) {
        LogError << "Inst is null";
        return;
    }
    auto action = inst_->custom_action(param.name);
    if (!action) {
        LogError << "Custom task not found" << VAR(param.name);
        return;
    }

    // TODO: 识别结果转 json
    action->run(param, cur_box, json::value());
}

cv::Rect PipelineTask::get_target_rect(const MAA_PIPELINE_RES_NS::Action::Target target, const cv::Rect& cur_box)
{
    using namespace MAA_PIPELINE_RES_NS::Action;

    if (!status()) {
        LogError << "Status is null";
        return {};
    }

    cv::Rect raw;
    switch (target.type) {
    case Target::Type::Self:
        raw = cur_box;
        break;
    case Target::Type::PreTask:
        raw = status()->get_pipeline_rec_cache(std::get<std::string>(target.param));
        break;
    case Target::Type::Region:
        raw = std::get<cv::Rect>(target.param);
        break;
    default:
        LogError << "Unknown target" << VAR(static_cast<int>(target.type));
        return {};
    }

    return cv::Rect { raw.x + target.offset.x, raw.y + target.offset.y, raw.width + target.offset.width,
                      raw.height + target.offset.height };
}

const MAA_PIPELINE_RES_NS::TaskData& PipelineTask::get_task_data(const std::string& task_name)
{
    auto modified_it = diff_tasks_.find(task_name);
    if (modified_it != diff_tasks_.end()) {
        return modified_it->second;
    }

    if (!resource()) {
        LogError << "Resource not binded";
        static MAA_PIPELINE_RES_NS::TaskData empty;
        return empty;
    }

    auto& data_mgr = resource()->pipeline_cfg();
    return data_mgr.get_task_data(task_name);
}

void PipelineTask::sleep(unsigned ms) const
{
    sleep(std::chrono::milliseconds(ms));
}

void PipelineTask::sleep(std::chrono::milliseconds ms) const
{
    if (need_exit()) {
        return;
    }

    using namespace std::chrono_literals;

    if (ms == 0ms) {
        std::this_thread::yield();
        return;
    }

    auto interval = std::min(ms, 5000ms);

    LogTrace << "ready to sleep" << ms << VAR(interval);

    for (auto sleep_time = interval; sleep_time <= ms && !need_exit(); sleep_time += interval) {
        std::this_thread::sleep_for(interval);
    }
    if (!need_exit()) {
        std::this_thread::sleep_for(ms % interval);
    }

    LogTrace << "end of sleep" << ms << VAR(interval);
}

MAA_TASK_NS_END
