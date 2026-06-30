#pragma once

#include <string>
#include <unordered_map>

#include "Common/Conf.h"
#include "MaaFramework/MaaDef.h"

struct MaaTasker;

MAA_PROJECT_INTERFACE_NS_BEGIN

// RAII wrapper: registers a stdout progress sink on construction, removes it on destruction.
// level 0: no-op (nothing registered)
// level 1: print Task Starting/Succeeded/Failed
// level 2: level 1 + Node Running/Succeeded/Failed
//
// Lifetime / ownership contract:
//   - 不持有 tasker 的所有权，仅保存裸指针。调用方必须保证传入的 tasker 在
//     ProgressSink 析构之前持续存活，否则析构时 MaaTaskerRemoveSink 将访问已释放对象 (UAF)。
//   - 析构会调用 MaaTaskerRemoveSink；MaaFW 工作线程仍可能在投递回调，
//     因此**应在所有 post_task 都已 wait 完成之后**再让 ProgressSink 离开作用域，
//     否则 on_event 与 RemoveSink 之间存在竞争窗口。
//   - on_event 是 C 风格回调，本类内部已对所有异常做兜底（不会逃出 C 边界）。
class ProgressSink
{
public:
    ProgressSink(MaaTasker* tasker, int level, std::unordered_map<std::string, std::string> task_names = {});
    ~ProgressSink();

    ProgressSink(const ProgressSink&) = delete;
    ProgressSink& operator=(const ProgressSink&) = delete;

private:
    static void on_event(void* handle, const char* message, const char* details_json, void* trans_arg);

    std::string task_label(const std::string& entry) const;

    MaaTasker* tasker_ = nullptr;
    MaaSinkId sink_id_ = MaaInvalidId;
    MaaSinkId context_sink_id_ = MaaInvalidId;
    int level_ = 0;
    std::unordered_map<std::string, std::string> task_names_;
};

MAA_PROJECT_INTERFACE_NS_END
