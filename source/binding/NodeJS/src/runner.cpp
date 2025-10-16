#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <MaaFramework/MaaAPI.h>
#include <MaaToolkit/MaaToolkitAPI.h>

int main(int argc, char* argv[])
{
    if (argc < 1) {
        std::cerr << "Usage: " << argv[0] << " [index.js]" << std::endl;
        return 1;
    }

    int level = MaaLoggingLevel_Warn;
    MaaGlobalSetOption(MaaGlobalOption_StdoutLevel, &level, sizeof(level));

    std::ifstream fin(argv[1]);
    std::stringstream buf;
    buf << fin.rdbuf();

    std::string source = buf.str();
    MaaStringBuffer* output = MaaStringBufferCreate();

    MaaToolkitRunQuickJS(source.c_str(), output);

    std::cout << MaaStringBufferGet(output) << std::endl;
    MaaStringBufferDestroy(output);
    return 0;
}
