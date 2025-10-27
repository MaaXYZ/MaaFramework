#pragma once

#include <string>

#include <MaaFramework/MaaAPI.h>

struct QuickJSRuntimeData;

class QuickJSRuntime
{
public:
    QuickJSRuntime();
    ~QuickJSRuntime();

    void eval_file(std::string file);
    void eval_script(std::string script);
    void exec_loop(bool auto_quit = true);
    std::string get_result();

    void dispatch_resource_sink(MaaResource* handle, const char* message, const char* details_json);
    void dispatch_controller_sink(MaaController* handle, const char* message, const char* details_json);
    void dispatch_tasker_sink(MaaTasker* handle, const char* message, const char* details_json);
    void dispatch_context_sink(MaaContext* handle, const char* message, const char* details_json);

private:
    QuickJSRuntimeData* d_;
};
