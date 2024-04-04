#pragma once

#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <string_view>
#include <thread>
#include <filesystem>

#include <meojson/json.hpp>

#include "Conf/Conf.h"
#include "ExecAgent/ExecArgConverter.h"
#include "ExecAgentType.h"
#include "MaaFramework/MaaDef.h"
#include "Utils/IOStream/ChildPipeIOStream.h"

MAA_TOOLKIT_NS_BEGIN

class ExecAgentBase
{
public:
    virtual ~ExecAgentBase();

    bool register_executor(
        MaaInstanceHandle handle,
        std::string_view name,
        std::filesystem::path exec_path,
        std::vector<std::string> exec_args);
    bool unregister_executor(MaaInstanceHandle handle, std::string_view name);

protected:
    virtual bool register_for_maa_inst(
        MaaInstanceHandle handle,
        std::string_view name,
        ExecData& executor) = 0;
    virtual bool unregister_for_maa_inst(MaaInstanceHandle handle, std::string_view name) = 0;

    std::optional<json::value> run_executor(
        const std::filesystem::path& exec_path,
        const std::vector<std::string>& exec_args);

protected:
    std::map<std::string, ExecData> exec_data_;

    ExecArgConverter arg_cvt_;

private:
    std::optional<json::value> handle_ipc(IOStream& ios);

    std::string handle_command(const json::value& cmd);

    json::value ctx_run_task(const json::value& cmd);
    json::value ctx_run_recognizer(const json::value& cmd);
    json::value ctx_run_action(const json::value& cmd);
    json::value ctx_click(const json::value& cmd);
    json::value ctx_swipe(const json::value& cmd);
    json::value ctx_press_key(const json::value& cmd);
    json::value ctx_input_text(const json::value& cmd);
    json::value ctx_touch_down(const json::value& cmd);
    json::value ctx_touch_move(const json::value& cmd);
    json::value ctx_touch_up(const json::value& cmd);
    json::value ctx_screencap(const json::value& cmd);

    MaaSyncContextHandle get_sync_context(const json::value& cmd);
    json::value invalid_json();
    json::value gen_result(bool success);

    void detach_child(ChildPipeIOStream&& child);
    void child_daemon();

private:
    std::vector<ChildPipeIOStream> detached_children_;

    bool child_daemon_quit_ = true;
    std::mutex child_daemon_mutex_;
    std::condition_variable child_daemon_condvar_;
    std::thread child_daemon_thread_;
};

MAA_TOOLKIT_NS_END
