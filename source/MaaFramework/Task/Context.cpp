#include "Context.h"

#include <atomic>
#include <future>
#include <limits>
#include <thread>

#include <meojson/json.hpp>

#include "ActionTask.h"
#include "Component/ActionHelper.h"
#include "Component/Recognizer.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Uuid.h"
#include "PipelineTask.h"
#include "RecognitionTask.h"
#include "Resource/PipelineChecker.h"
#include "Resource/PipelineDumper.h"
#include "Resource/PipelineParser.h"
#include "Tasker/Tasker.h"

MAA_TASK_NS_BEGIN

std::shared_ptr<Context> Context::create(MaaTaskId id, Tasker* tasker)
{
    LogDebug << VAR(id) << VAR_VOIDP(tasker);

    return std::make_shared<Context>(id, tasker, PrivateArg { });
}

std::shared_ptr<Context> Context::getptr()
{
    return shared_from_this();
}

std::shared_ptr<const Context> Context::getptr() const
{
    return shared_from_this();
}

std::shared_ptr<Context> Context::make_clone() const
{
    return std::make_shared<Context>(*this);
}

Context::Context(MaaTaskId id, Tasker* tasker, PrivateArg)
    : task_id_(id)
    , tasker_(tasker)
    , task_state_(std::make_shared<TaskState>())
    , need_to_stop_(std::make_shared<bool>())
{
    LogDebug << VAR(id) << VAR_VOIDP(tasker);
}

Context::Context(const Context& other)
    : std::enable_shared_from_this<Context>(other)
    , task_id_(other.task_id_)
    , tasker_(other.tasker_)
    , pipeline_override_(other.pipeline_override_)
    , image_override_(other.image_override_)
    , task_state_(other.task_state_)
    , need_to_stop_(other.need_to_stop_)
// don't copy clone_holder_
{
    LogDebug << VAR(other.getptr());
}

MaaTaskId Context::run_task(const std::string& entry, const json::value& pipeline_override)
{
    LogTrace << VAR(getptr()) << VAR(entry) << VAR(pipeline_override);

    if (!tasker_) {
        LogError << "tasker is null";
        return MaaInvalidId;
    }

    PipelineTask subtask(entry, tasker_, make_clone());
    bool ov = subtask.override_pipeline(pipeline_override);
    if (!ov) {
        LogError << "failed to override_pipeline" << VAR(entry) << VAR(pipeline_override);
        return MaaInvalidId;
    }

    auto& runtime_cache = tasker_->runtime_cache();

    // context 的子任务没有 Pending 状态，直接就是 Running
    runtime_cache.set_task_detail(
        subtask.task_id(),
        MAA_TASK_NS::TaskDetail { .task_id = subtask.task_id(), .entry = subtask.entry(), .status = MaaStatus_Running });

    bool ret = subtask.run();

    {
        auto task_detail = runtime_cache.get_task_detail(subtask.task_id())
                               .value_or(MAA_TASK_NS::TaskDetail { .task_id = subtask.task_id(), .entry = entry });
        task_detail.status = ret ? MaaStatus_Succeeded : MaaStatus_Failed;
        runtime_cache.set_task_detail(subtask.task_id(), std::move(task_detail));
    }

    return subtask.task_id();
}

MaaRecoId Context::run_recognition(const std::string& entry, const json::value& pipeline_override, const cv::Mat& image)
{
    LogTrace << VAR(getptr()) << VAR(entry) << VAR(pipeline_override);

    RecognitionTask subtask(image, entry, tasker_, make_clone());
    bool ov = subtask.override_pipeline(pipeline_override);
    if (!ov) {
        LogError << "failed to override_pipeline" << VAR(entry) << VAR(pipeline_override);
        return MaaInvalidId;
    }
    return subtask.run_impl();
}

int64_t Context::run_recognition_list(const std::vector<std::string>& entries, const json::value& pipeline_override, const cv::Mat& image_in)
{
    LogFunc << VAR(getptr()) << VAR(entries) << VAR(pipeline_override);

    if (entries.empty()) {
        return -1;
    }
    if (!tasker_) {
        LogError << "tasker is null";
        return -1;
    }

    // 用一个 clone 承载可选的 override，识别期间只读，供多线程共享。
    auto work_ctx = make_clone();
    if (!pipeline_override.empty() && !work_ctx->override_pipeline(pipeline_override)) {
        LogError << "failed to override_pipeline" << VAR(pipeline_override);
        return -1;
    }

    // 与 run_recognition 一致：画面由调用方传入，所有 entry 共用同一画面，本函数不负责抓取。
    const cv::Mat& image = image_in;
    if (image.empty()) {
        LogError << "empty image";
        return -1;
    }

    const size_t count = entries.size();

    // 预取每个 entry 的 pipeline 数据并按"识别类型是否会触发 Agent 回调"分流（串行，开销很小）：
    //   - Custom 识别在主进程里会经由 reco_agent 反向 IPC 调用 Agent，必须串行跑在当前 IPC 处理线程上，
    //     绝不能塞进 worker 线程（多线程同时占用同一 socket + 嵌套 send/recv 会冲垮协议）。
    //   - 其余纯本地识别（TemplateMatch/OCR/ColorMatch/And/Or 等）无副作用，可安全并行。
    // 注意：若 And/Or 的子识别引用了 Custom 节点，同样会触发回调；实时任务当前不存在该用法，故只判顶层类型。
    std::vector<PipelineData> datas(count);
    std::vector<size_t> parallel_indices; // 升序，非 Custom
    std::vector<size_t> custom_indices;    // 升序，Custom
    for (size_t i = 0; i < count; ++i) {
        auto data_opt = work_ctx->get_pipeline_data(entries[i]);
        if (!data_opt) {
            LogWarn << "node not found, skip" << VAR(entries[i]);
            continue;
        }
        if (!data_opt->enabled) {
            continue;
        }
        datas[i] = std::move(*data_opt);
        if (datas[i].reco_type == MAA_RES_NS::Recognition::Type::Custom) {
            custom_indices.emplace_back(i);
        }
        else {
            parallel_indices.emplace_back(i);
        }
    }

    // best 记录已命中的最小 entry 下标，保留 next 列表"下标即优先级"的语义。
    std::atomic<int64_t> best { std::numeric_limits<int64_t>::max() };

    // 直接走 Recognizer，绕过 RecognitionTask 的逐节点回调（避免并发回调竞态与日志洪泛）。
    // 与 TaskBase::run_recognition 一致：node 级 inverse 需在此翻转命中结果。
    auto recognize_hit = [&](size_t i) -> bool {
        Recognizer recognizer(tasker_, *work_ctx, image);
        RecoResult res = recognizer.recognize(datas[i].reco_type, datas[i].reco_param, datas[i].name);
        bool hit = res.box.has_value();
        if (datas[i].inverse) {
            hit = !hit;
        }
        return hit;
    };

    // 并行阶段：只跑非 Custom 节点。parallel_indices 升序，故 worker 跨步遍历到的下标也升序，
    // 一旦当前下标已高于已命中的 best，后续都不可能成为更高优先级命中，直接 break 提前退出。
    const size_t parallel_count = parallel_indices.size();
    const unsigned hardware_threads = std::max(1U, std::thread::hardware_concurrency());
    const size_t worker_count = std::min(parallel_count, static_cast<size_t>(hardware_threads));

    auto worker = [&](size_t start) {
        for (size_t k = start; k < parallel_count; k += worker_count) {
            const size_t i = parallel_indices[k];
            if (static_cast<int64_t>(i) > best.load(std::memory_order_relaxed)) {
                break;
            }
            if (!recognize_hit(i)) {
                continue;
            }
            int64_t prev = best.load(std::memory_order_relaxed);
            while (static_cast<int64_t>(i) < prev
                   && !best.compare_exchange_weak(prev, static_cast<int64_t>(i), std::memory_order_relaxed)) {
            }
            break;
        }
    };

    if (worker_count <= 1) {
        if (worker_count == 1) {
            worker(0);
        }
    }
    else {
        std::vector<std::future<void>> futures;
        futures.reserve(worker_count - 1);
        for (size_t w = 1; w < worker_count; ++w) {
            futures.emplace_back(std::async(std::launch::async, worker, w));
        }
        worker(0);
        for (auto& f : futures) {
            f.get();
        }
    }

    // 串行阶段：Custom 节点按下标升序逐个跑，只在其优先级高于已命中 best 时才识别；
    // 命中即为剩余范围内的最高优先级，直接收敛。
    for (size_t i : custom_indices) {
        if (static_cast<int64_t>(i) >= best.load(std::memory_order_relaxed)) {
            break;
        }
        if (recognize_hit(i)) {
            best.store(static_cast<int64_t>(i), std::memory_order_relaxed);
            break;
        }
    }

    const int64_t hit = best.load(std::memory_order_relaxed);
    const int64_t hit_index = hit == std::numeric_limits<int64_t>::max() ? -1 : hit;
    LogTrace << VAR(getptr()) << VAR(hit_index) << VAR(count);
    return hit_index;
}

MaaActId
    Context::run_action(const std::string& entry, const json::value& pipeline_override, const cv::Rect& box, const std::string& reco_detail)
{
    LogTrace << VAR(getptr()) << VAR(entry) << VAR(pipeline_override) << VAR(box) << VAR(reco_detail);

    ActionTask subtask(box, reco_detail, entry, tasker_, make_clone());
    bool ov = subtask.override_pipeline(pipeline_override);
    if (!ov) {
        LogError << "failed to override_pipeline" << VAR(entry) << VAR(pipeline_override);
        return MaaInvalidId;
    }
    return subtask.run_impl();
}

MaaRecoId Context::run_recognition_direct(const std::string& reco_type, const json::value& reco_param, const cv::Mat& image)
{
    LogTrace << VAR(getptr()) << VAR(reco_type) << VAR(reco_param);

    std::string entry = std::format("recognition/{}/{}", reco_type, make_uuid());

    json::value pipeline_override;
    pipeline_override[entry]["recognition"] = { { "type", reco_type }, { "param", reco_param } };

    return run_recognition(entry, pipeline_override, image);
}

MaaActId Context::run_action_direct(
    const std::string& action_type,
    const json::value& action_param,
    const cv::Rect& box,
    const std::string& reco_detail)
{
    LogTrace << VAR(getptr()) << VAR(action_type) << VAR(action_param) << VAR(box) << VAR(reco_detail);

    std::string entry = std::format("action/{}/{}", action_type, make_uuid());

    json::value pipeline_override;
    pipeline_override[entry]["action"] = { { "type", action_type }, { "param", action_param } };

    return run_action(entry, pipeline_override, box, reco_detail);
}

bool Context::wait_freezes(std::chrono::milliseconds time, const cv::Rect& box, const json::value& wait_freezes_param)
{
    LogTrace << VAR(getptr()) << VAR(time) << VAR(box) << VAR(wait_freezes_param);

    if (!tasker_) {
        LogError << "tasker is null";
        return false;
    }
    auto* resource = tasker_->resource();
    if (!resource) {
        LogError << "resource is null";
        return false;
    }

    // 从 DefaultPipelineMgr 获取默认值
    const auto& default_param = resource->default_pipeline().get_pipeline().pre_wait_freezes;

    // 解析 wait_freezes_param
    MAA_RES_NS::WaitFreezesParam param;
    if (!MAA_RES_NS::PipelineParser::parse_wait_freezes_value(wait_freezes_param, param, default_param)) {
        LogError << "failed to parse wait_freezes_param" << VAR(wait_freezes_param);
        return false;
    }

    // 校验并合并 time：两者互斥
    if (time > std::chrono::milliseconds(0) && param.time > std::chrono::milliseconds(0)) {
        LogError << "time and wait_freezes_param.time are mutually exclusive, both are non-zero" << VAR(time) << VAR(param.time);
        return false;
    }
    if (time > std::chrono::milliseconds(0)) {
        param.time = time;
    }
    if (param.time <= std::chrono::milliseconds(0)) {
        LogError << "time is required but not provided" << VAR(time) << VAR(param.time);
        return false;
    }

    ActionHelper helper(this);
    return helper.wait_freezes(param, box, { .phase = "context" });
}

bool Context::override_pipeline(const json::value& pipeline_override)
{
    LogTrace << VAR(getptr()) << VAR(pipeline_override);

    if (pipeline_override.empty()) {
        return true;
    }

    if (!tasker_) {
        LogError << "tasker is null";
        return false;
    }
    auto* resource = tasker_->resource();
    if (!resource) {
        LogError << "resource not bound";
        return false;
    }
    auto& default_mgr = resource->default_pipeline();

    bool ret = false;
    if (pipeline_override.is_object()) {
        ret = override_pipeline_once(pipeline_override.as_object(), default_mgr);
    }
    else if (pipeline_override.is_array()) {
        ret = true;
        for (const auto& val : pipeline_override.as_array()) {
            if (!val.is_object()) {
                LogError << "input is not json array of object" << VAR(pipeline_override);
                return false;
            }
            ret &= override_pipeline_once(val.as_object(), default_mgr);
        }
    }
    else {
        LogError << "input is invalid" << VAR(pipeline_override);
        return false;
    }

    return ret && check_pipeline();
}

bool Context::override_next(const std::string& node_name, const std::vector<std::string>& next)
{
    LogTrace << VAR(getptr()) << VAR(node_name) << VAR(next);

    auto data_opt = get_pipeline_data(node_name);
    if (!data_opt) {
        LogError << "get_pipeline_data failed, task not exist" << VAR(node_name);
        return false;
    }

    if (!MAA_RES_NS::PipelineParser::parse_next(next, data_opt->next)) {
        LogError << "failed to parse_next" << VAR(next);
        return false;
    }

    pipeline_override_.insert_or_assign(node_name, std::move(*data_opt));

    return check_pipeline();
}

bool Context::override_image(const std::string& image_name, const cv::Mat& image)
{
    LogInfo << VAR(getptr()) << VAR(image_name) << VAR(image);

    image_override_.insert_or_assign(image_name, image);
    return true;
}

std::optional<json::object> Context::get_node_data(const std::string& node_name) const
{
    auto pp_opt = get_pipeline_data(node_name);
    if (!pp_opt) {
        return std::nullopt;
    }

    return MAA_RES_NS::PipelineDumper::dump(*pp_opt);
}

Context* Context::clone() const
{
    auto& ref = clone_holder_.emplace_back(make_clone());
    LogDebug << VAR(getptr()) << VAR(ref);

    return ref.get();
}

MaaTaskId Context::task_id() const
{
    return task_id_;
}

Tasker* Context::tasker() const
{
    return tasker_;
}

size_t Context::get_hit_count(const std::string& node_name) const
{
    auto it = task_state_->hit_count.find(node_name);
    if (it != task_state_->hit_count.end()) {
        return it->second;
    }
    return 0;
}

void Context::clear_hit_count(const std::string& node_name)
{
    task_state_->hit_count.erase(node_name);
}

void Context::set_anchor(const std::string& anchor_name, const std::string& node_name)
{
    LogDebug << VAR(anchor_name) << VAR(node_name);
    if (node_name.empty()) {
        task_state_->anchors.erase(anchor_name);
    }
    else {
        task_state_->anchors[anchor_name] = node_name;
    }
}

std::optional<std::string> Context::get_anchor(const std::string& anchor_name) const
{
    auto it = task_state_->anchors.find(anchor_name);
    if (it != task_state_->anchors.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<PipelineData> Context::get_pipeline_data(const std::string& node_name) const
{
    auto override_it = pipeline_override_.find(node_name);
    if (override_it != pipeline_override_.end()) {
        LogDebug << "found in override" << VAR(node_name);
        return override_it->second;
    }

    if (!tasker_) {
        LogError << "tasker is null";
        return std::nullopt;
    }
    auto* resource = tasker_->resource();
    if (!resource) {
        LogError << "resource not bound";
        return std::nullopt;
    }

    const auto& raw_pp_map = resource->pipeline_res().get_pipeline_data_map();
    auto raw_it = raw_pp_map.find(node_name);
    if (raw_it != raw_pp_map.end()) {
        return raw_it->second;
    }

    LogWarn << "task not found" << VAR(node_name);
    return std::nullopt;
}

std::optional<PipelineData> Context::get_pipeline_data(const MAA_RES_NS::NodeAttr& node_attr) const
{
    std::string node_name = node_attr.name;
    if (node_attr.anchor) {
        auto anchor_node = get_anchor(node_attr.name);
        if (!anchor_node) {
            LogDebug << "anchor not set" << VAR(node_attr.name);
            return std::nullopt;
        }
        node_name = *anchor_node;
    }
    return get_pipeline_data(node_name);
}

std::vector<cv::Mat> Context::get_images(const std::vector<std::string>& names)
{
    if (!tasker_) {
        LogError << "tasker is null";
        return { };
    }
    auto* resource = tasker_->resource();
    if (!resource) {
        LogError << "resource not bound";
        return { };
    }

    std::vector<cv::Mat> results;

    for (const std::string& name : names) {
        auto it = image_override_.find(name);
        if (it != image_override_.end()) {
            LogTrace << "image override" << VAR(name);
            results.emplace_back(it->second);
            continue;
        }

        auto imgs = resource->template_res().get_image(name);
        results.insert(results.end(), std::make_move_iterator(imgs.begin()), std::make_move_iterator(imgs.end()));
    }

    return results;
}

bool& Context::need_to_stop()
{
    return *need_to_stop_;
}

bool Context::check_hit_count(const PipelineData& data)
{
    size_t current_hit = get_hit_count(data.name);
    if (current_hit >= static_cast<size_t>(data.max_hit)) {
        LogDebug << "max_hit reached" << VAR(data.name) << VAR(current_hit) << VAR(data.max_hit);
        return false;
    }
    return true;
}

void Context::increment_hit_count(const std::string& node_name)
{
    task_state_->hit_count[node_name]++;
}

bool Context::override_pipeline_once(const json::object& pipeline_override, const MAA_RES_NS::DefaultPipelineMgr& default_mgr)
{
    // LogTrace << VAR(getptr()) << VAR(pipeline_override);

    for (const auto& [key, value] : pipeline_override) {
        PipelineData result;
        auto default_result = get_pipeline_data(key).value_or(default_mgr.get_pipeline());
        bool ret = MAA_RES_NS::PipelineParser::parse_node(key, value, result, default_result, default_mgr);
        if (!ret) {
            LogError << "parse_task failed" << VAR(key) << VAR(value);
            return false;
        }

        pipeline_override_.insert_or_assign(key, std::move(result));
    }

    return true;
}

bool Context::check_pipeline() const
{
    if (!tasker_) {
        LogError << "tasker is null";
        return false;
    }
    auto* resource = tasker_->resource();
    if (!resource) {
        LogError << "resource not bound";
        return false;
    }

    auto raw = resource->pipeline_res().get_pipeline_data_map();
    auto all = pipeline_override_;
    all.merge(raw);

    return MAA_RES_NS::PipelineChecker::check_all_validity(all);
}

MAA_TASK_NS_END
