#include <iostream>

#include <MaaFramework/MaaAPI.h>
#include <MaaToolkit/MaaToolkitAPI.h>

#include "runtime.h"

int main(int argc, char* argv[])
{
    if (argc < 1) {
        std::cerr << "Usage: " << argv[0] << " [index.js]" << std::endl;
        return 1;
    }

    int level = MaaLoggingLevel_Warn;
    MaaGlobalSetOption(MaaGlobalOption_StdoutLevel, &level, sizeof(level));

    QuickJSRuntime runtime;
    runtime.eval_file(argv[1]);
    runtime.exec_loop();

    std::cout << runtime.get_result() << std::endl;
}
