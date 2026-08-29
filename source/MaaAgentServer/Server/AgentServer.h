#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <meojson/json.hpp>
#include <zmq.hpp>

#include "Common/Conf.h"
#include "MaaAgent/Transceiver.h"
#include "MaaAgentServer/MaaAgentServerDef.h"
#include "MaaUtils/SingletonHolder.hpp"
#include "Utils/EventDispatcher.hpp"

MAA_AGENT_SERVER_NS_BEGIN

class AgentServer
    : public SingletonHolder<AgentServer>
    , public Transceiver
{
    struct CustomRecognitionSession
    {
        MaaCustomRecognitionCallback recognition = nullptr;
        void* trans_arg = nullptr;
    };

    struct CustomActionSession
    {
        MaaCustomActionCallback action = nullptr;
        void* trans_arg = nullptr;
    };

    struct ShutdownSession
    {
        MaaShutdownCallback callback = nullptr;
        void* trans_arg = nullptr;
    };

public:
    ~AgentServer() = default;

    bool start_up(const std::string& identifier);
    void shut_down();
    void join();
    void detach();

    bool register_custom_recognition(const std::string& name, MaaCustomRecognitionCallback recognition, void* trans_arg);
    bool register_custom_action(const std::string& name, MaaCustomActionCallback action, void* trans_arg);
    bool set_shutdown_callback(MaaShutdownCallback callback, void* trans_arg);

    MaaSinkId add_resource_sink(MaaEventCallback sink, void* trans_arg);
    MaaSinkId add_controller_sink(MaaEventCallback sink, void* trans_arg);
    MaaSinkId add_tasker_sink(MaaEventCallback sink, void* trans_arg);
    MaaSinkId add_context_sink(MaaEventCallback sink, void* trans_arg);

public:
    virtual bool handle_inserted_request(const json::value& j) override;

private:
    bool handle_recognition_request(const json::value& j);
    bool handle_action_request(const json::value& j);
    bool handle_start_up_request(const json::value& j);
    bool handle_shut_down_request(const json::value& j);

    bool handle_resource_event(const json::value& j);
    bool handle_controller_event(const json::value& j);
    bool handle_tasker_event(const json::value& j);
    bool handle_context_event(const json::value& j);

    void request_msg_loop();

private:
    std::unordered_map<std::string, CustomRecognitionSession> custom_recognitions_;
    std::unordered_map<std::string, CustomActionSession> custom_actions_;

    ShutdownSession shutdown_session_;

    EventDispatcher res_notifier_;
    EventDispatcher ctrl_notifier_ = EventDispatcher(false);
    EventDispatcher tasker_notifier_;
    EventDispatcher ctx_notifier_;

    // "消息循环是否该继续跑"的开关。宿主线程（ShutDown/本地关停）写 false，
    // 消息线程（while 条件 + recv 的中断谓词）读。跨线程读写，用原子保证可见性。
    std::atomic<bool> msg_loop_running_ = false;
    std::thread msg_thread_;
    // shut_down / join / detach 可能从不同线程并发调用（典型场景：主线程
    // 正在 join() 等待服务结束，另一个线程调用 ShutDown() 请求关停）。
    // 这把锁保证 joinable() 检查和 join()/detach() 是原子的——没有它的话，
    // 两个线程可能同时通过 joinable 检查、对同一线程 join 两次（未定义行为）。
    std::mutex msg_thread_mutex_;
};

MAA_AGENT_SERVER_NS_END
