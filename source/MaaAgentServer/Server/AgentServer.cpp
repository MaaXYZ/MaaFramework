#include "AgentServer.h"

#include <ranges>

#include "MaaAgent/Message.hpp"
#include "MaaUtils/Buffer/ImageBuffer.hpp"
#include "MaaUtils/Buffer/StringBuffer.hpp"
#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"
#include "RemoteInstance/RemoteContext.h"

MAA_AGENT_SERVER_NS_BEGIN

bool AgentServer::start_up(const std::string& identifier)
{
    LogFunc << VAR(identifier);

    if (identifier.empty()) {
        LogError << "identifier is empty";
        return false;
    }

    auto port_opt = parse_tcp_port(identifier);

    if (port_opt) {
        LogInfo << "Using TCP mode" << VAR(*port_opt);
        init_tcp_socket(*port_opt, false);
    }
    else {
        init_socket(identifier, false);
    }

    msg_loop_running_ = true;

    // 覆盖消息线程全部等待路径（send/recv/嵌套 send_and_recv）的中断开关：
    // 对端消失后 POLLOUT/POLLIN 永不就绪且 timeout_ 默认无限，没有它，
    // 本地 ShutDown 的 join 会随任一等待永久挂死（须在消息线程启动前设置）
    set_abort_pred([this] { return !msg_loop_running_.load(); });

    msg_thread_ = std::thread(&AgentServer::request_msg_loop, this);
    if (!msg_thread_.joinable()) {
        LogError << "failed to start msg_thread";
        return false;
    }

    return true;
}

void AgentServer::shut_down()
{
    LogFunc << VAR(ipc_addr_);

    // 先置 false（锁外）：让消息线程的 poll 谓词立即看到"该停了"，
    // 这样下面 join 最多等一个 poll 周期（约 1 秒）就能返回。
    // 如果放在锁内：另一线程正在 join() 持锁等待 → 我们在这里等锁 →
    // 消息线程收不到中断信号 → join 永远不返回 → 死锁。
    msg_loop_running_ = false;

    {
        std::lock_guard lock(msg_thread_mutex_);
        if (msg_thread_.joinable()) {
            msg_thread_.join();
        }

        zmq_sock_.close();
        zmq_ctx_.close();
    }
}

void AgentServer::join()
{
    LogFunc << VAR(ipc_addr_);

    std::lock_guard lock(msg_thread_mutex_);
    if (!msg_thread_.joinable()) {
        LogError << "msg_thread is not joinable";
        return;
    }

    msg_thread_.join();
}

void AgentServer::detach()
{
    LogFunc << VAR(ipc_addr_);

    std::lock_guard lock(msg_thread_mutex_);
    if (!msg_thread_.joinable()) {
        LogError << "msg_thread is not joinable";
        return;
    }

    msg_thread_.detach();
}

bool AgentServer::register_custom_recognition(const std::string& name, MaaCustomRecognitionCallback recognition, void* trans_arg)
{
    LogInfo << VAR(name) << VAR_VOIDP(recognition) << VAR_VOIDP(trans_arg);

    if (name.empty() || recognition == nullptr) {
        LogError << "name or recognition is null";
        return false;
    }

    return custom_recognitions_.insert_or_assign(name, CustomRecognitionSession { recognition, trans_arg }).second;
}

bool AgentServer::register_custom_action(const std::string& name, MaaCustomActionCallback action, void* trans_arg)
{
    LogInfo << VAR(name) << VAR_VOIDP(action) << VAR_VOIDP(trans_arg);

    if (name.empty() || action == nullptr) {
        LogError << "name or action is null";
        return false;
    }

    return custom_actions_.insert_or_assign(name, CustomActionSession { action, trans_arg }).second;
}

bool AgentServer::set_shutdown_callback(MaaShutdownCallback callback, void* trans_arg)
{
    LogInfo << VAR_VOIDP(callback) << VAR_VOIDP(trans_arg);

    if (callback == nullptr) {
        LogError << "callback is null";
        return false;
    }

    // 必须在 start_up 之前设置：运行中修改的话，消息线程正在读 shutdown_session_
    // 而宿主线程同时在写——两边没有任何同步，并发读写是数据竞争（未定义行为）
    if (msg_loop_running_) {
        LogError << "server is running, set_shutdown_callback must be called before start_up";
        return false;
    }

    // 直接覆盖：重复注册时后设置的回调替换先设置的，不会叠加调用
    shutdown_session_ = ShutdownSession { callback, trans_arg };
    return true;
}

MaaSinkId AgentServer::add_resource_sink(MaaEventCallback sink, void* trans_arg)
{
    return res_notifier_.add_sink(sink, trans_arg);
}

MaaSinkId AgentServer::add_controller_sink(MaaEventCallback sink, void* trans_arg)
{
    return ctrl_notifier_.add_sink(sink, trans_arg);
}

MaaSinkId AgentServer::add_tasker_sink(MaaEventCallback sink, void* trans_arg)
{
    return tasker_notifier_.add_sink(sink, trans_arg);
}

MaaSinkId AgentServer::add_context_sink(MaaEventCallback sink, void* trans_arg)
{
    return ctx_notifier_.add_sink(sink, trans_arg);
}

bool AgentServer::handle_inserted_request(const json::value& j)
{
    // LogInfo << VAR(j) << VAR(ipc_addr_);

    if (handle_image_header(j)) {
        return true;
    }
    else if (handle_image_encoded_header(j)) {
        return true;
    }
    else if (handle_recognition_request(j)) {
        return true;
    }
    else if (handle_action_request(j)) {
        return true;
    }
    else if (handle_resource_event(j)) {
        return true;
    }
    else if (handle_controller_event(j)) {
        return true;
    }
    else if (handle_tasker_event(j)) {
        return true;
    }
    else if (handle_context_event(j)) {
        return true;
    }
    else if (handle_start_up_request(j)) {
        return true;
    }
    else if (handle_shut_down_request(j)) {
        return true;
    }
    else {
        LogError << "unexpected msg" << VAR(j);
        return false;
    }
}

bool AgentServer::handle_recognition_request(const json::value& j)
{
    if (!j.is<CustomRecognitionRequest>()) {
        return false;
    }

    const CustomRecognitionRequest& req = j.as<CustomRecognitionRequest>();
    LogInfo << VAR(req) << VAR(ipc_addr_);

    auto it = custom_recognitions_.find(req.custom_recognition_name);
    if (it == custom_recognitions_.end()) {
        LogError << "custom_recognition not found" << VAR(req);
        return true;
    }

    const CustomRecognitionSession& session = it->second;
    if (!session.recognition) {
        LogError << "recognition is null" << VAR(req);
        return true;
    }

    RemoteContext context(*this, req.context_id);
    cv::Mat mat = get_image_cache(req.image);
    ImageBuffer mat_buffer(mat);
    MaaRect rect { req.roi[0], req.roi[1], req.roi[2], req.roi[3] };

    MaaRect out_box { };
    StringBuffer out_detail;

    MaaBool ret = session.recognition(
        &context,
        req.task_id,
        req.node_name.c_str(),
        req.custom_recognition_name.c_str(),
        req.custom_recognition_param.c_str(),
        &mat_buffer,
        &rect,
        session.trans_arg,
        &out_box,
        &out_detail);

    CustomRecognitionResponse resp {
        .ret = static_cast<bool>(ret),
        .out_box = { out_box.x, out_box.y, out_box.width, out_box.height },
        .out_detail = out_detail.get(),
    };
    LogInfo << VAR(resp) << VAR(ipc_addr_);

    send(resp);

    return true;
}

bool AgentServer::handle_action_request(const json::value& j)
{
    if (!j.is<CustomActionRequest>()) {
        return false;
    }

    const CustomActionRequest& req = j.as<CustomActionRequest>();
    LogInfo << VAR(req) << VAR(ipc_addr_);

    auto it = custom_actions_.find(req.custom_action_name);
    if (it == custom_actions_.end()) {
        LogError << "custom_action not found" << VAR(req);
        return true;
    }

    const CustomActionSession& session = it->second;
    if (!session.action) {
        LogError << "action is null" << VAR(req);
        return true;
    }

    RemoteContext context(*this, req.context_id);
    MaaRect rect { req.box[0], req.box[1], req.box[2], req.box[3] };

    MaaBool ret = session.action(
        &context,
        req.task_id,
        req.node_name.c_str(),
        req.custom_action_name.c_str(),
        req.custom_action_param.c_str(),
        req.reco_id,
        &rect,
        session.trans_arg);

    CustomActionResponse resp {
        .ret = static_cast<bool>(ret),
    };
    LogInfo << VAR(resp) << VAR(ipc_addr_);

    send(resp);

    return true;
}

bool AgentServer::handle_start_up_request(const json::value& j)
{
    if (!j.is<StartUpRequest>()) {
        return false;
    }

    const StartUpRequest& req = j.as<StartUpRequest>();
    LogInfo << VAR(req) << VAR(ipc_addr_);

    if (req.protocol != kProtocolVersion) {
        LogError << "Protocol version mismatch" << "client:" << VAR(req.version) << VAR(req.protocol) << "server:" << VAR(MAA_VERSION)
                 << VAR(kProtocolVersion) << VAR(ipc_addr_);
        LogError << "Please update" << (req.protocol < kProtocolVersion ? "AgentClient" : "AgentServer");
    }

    auto action_names = custom_actions_ | std::views::keys;
    auto reco_names = custom_recognitions_ | std::views::keys;

    StartUpResponse msg {
        .actions = { action_names.begin(), action_names.end() },
        .recognitions = { reco_names.begin(), reco_names.end() },
    };

    return send(msg);
}

bool AgentServer::handle_shut_down_request(const json::value& j)
{
    if (!j.is<ShutDownRequest>()) {
        return false;
    }

    LogInfo << VAR(ipc_addr_);

    // 先执行宿主注册的关闭回调，让 agent 有机会在退出前完成收尾
    // （如冲走在途通知、写状态文件）——回调阻塞多久，响应就推迟多久
    if (shutdown_session_.callback) {
        LogInfo << "shutdown callback begin" << VAR_VOIDP(shutdown_session_.trans_arg);
        shutdown_session_.callback(shutdown_session_.trans_arg);
        LogInfo << "shutdown callback end";
    }

    // 先回响应再停循环。顺序不能反：poll() 每次醒来都会检查中断谓词
    // （即 !msg_loop_running_），如果先置 false，下面这行 send 的 poll
    // 会被自己的谓词掐断——响应永远发不出去，客户端的 disconnect()
    // 只能等超时（默认无限）。
    //
    // 对端已死的情况：send 会等 POLLOUT（等不到），此时本地 ShutDown()
    // 置 false 后谓词转为 true，send 被"合理地"打断——这正是我们想要的。
    send(ShutDownResponse { });

    msg_loop_running_ = false;

    return true;
}

bool AgentServer::handle_resource_event(const json::value& j)
{
    if (!j.is<ResourceEventRequest>()) {
        return false;
    }

    const ResourceEventRequest& req = j.as<ResourceEventRequest>();
    // LogFunc << VAR(req) << VAR(ipc_addr_) << VAR(req.message);

    RemoteResource resource(*this, req.resource_id);
    res_notifier_.notify(&resource, req.message, req.details);

    send(ResourceEventResponse { });

    return true;
}

bool AgentServer::handle_controller_event(const json::value& j)
{
    if (!j.is<ControllerEventRequest>()) {
        return false;
    }
    const ControllerEventRequest& req = j.as<ControllerEventRequest>();
    // LogFunc << VAR(req) << VAR(ipc_addr_) << VAR(req.message);

    RemoteController controller(*this, req.controller_id);
    ctrl_notifier_.notify(&controller, req.message, req.details);

    send(ControllerEventResponse { });

    return true;
}

bool AgentServer::handle_tasker_event(const json::value& j)
{
    if (!j.is<TaskerEventRequest>()) {
        return false;
    }
    const TaskerEventRequest& req = j.as<TaskerEventRequest>();
    // LogFunc << VAR(req) << VAR(ipc_addr_) << VAR(req.message);

    RemoteTasker tasker(*this, req.tasker_id);
    tasker_notifier_.notify(&tasker, req.message, req.details);

    send(TaskerEventResponse { });

    return true;
}

bool AgentServer::handle_context_event(const json::value& j)
{
    if (!j.is<ContextEventRequest>()) {
        return false;
    }
    const ContextEventRequest& req = j.as<ContextEventRequest>();
    // LogFunc << VAR(req) << VAR(ipc_addr_) << VAR(req.message);

    RemoteContext context(*this, req.context_id);
    ctx_notifier_.notify(&context, req.message, req.details);

    send(ContextEventResponse { });

    return true;
}

void AgentServer::request_msg_loop()
{
    LogFunc << VAR(ipc_addr_);

    while (msg_loop_running_) {
        // recv 传入"该不该停"的检查函数，poll 每秒醒来问一次：
        // 本地 ShutDown() 置 false 后，最多 1 秒就能打断阻塞中的 recv。
        // 没有它的话，客户端不发关闭请求就消失时，recv 会以默认无限
        // 超时永远等下去——Join() 也跟着永远不返回，进程残留。
        auto msg_opt = recv([this] { return !msg_loop_running_; });
        if (!msg_opt) {
            // recv 失败有两种原因：被上面的谓词打断（正常关停，静默退出）
            // 或真正的通信错误（打日志后退出）
            if (!msg_loop_running_) {
                return;
            }
            LogError << "failed to recv msg" << VAR(ipc_addr_);
            return;
        }
        const json::value& j = *msg_opt;
        handle_inserted_request(j);
    }
}

MAA_AGENT_SERVER_NS_END
