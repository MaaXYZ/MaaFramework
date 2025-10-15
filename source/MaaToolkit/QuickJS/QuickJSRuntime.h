#pragma once

#include <string>

#include "Conf/Conf.h"

MAA_TOOLKIT_NS_BEGIN

struct QuickJSRuntimeData;

class QuickJSRuntime
{
public:
    QuickJSRuntime();
    ~QuickJSRuntime();

    void eval_script(std::string script);
    void exec_loop();
    std::string get_result();

private:
    QuickJSRuntimeData* d_;
};

MAA_TOOLKIT_NS_END
