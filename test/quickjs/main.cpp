// QuickJS 绑定的最小宿主：加载脚本 → 执行 → 按脚本返回的结果决定退出码。
// 脚本以 exit('ok') / exit('skip: ...') / exit('fail: ...') 汇报。

#include <cstdio>
#include <string>

#include "runtime.h"

namespace
{
constexpr int kSkipExitCode = 77;
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::printf("usage: quickjs_runtime_test <script.js>\n");
        return 2;
    }

    QuickJSRuntime runtime;
    runtime.eval_file(argv[1]);
    runtime.exec_loop(true);

    const std::string result = runtime.get_result();
    std::printf("quickjs result: %s\n", result.c_str());

    if (result == "ok") {
        return 0;
    }
    if (result.rfind("skip:", 0) == 0) {
        return kSkipExitCode;
    }
    return 1;
}
