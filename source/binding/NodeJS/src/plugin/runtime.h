#pragma once

#include <string>

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

private:
    QuickJSRuntimeData* d_;
};
