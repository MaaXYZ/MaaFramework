#include "MaaAgentClient/MaaAgentClientAPI.h"
#include "MaaFramework/MaaAPI.h"
#include "MaaToolkit/MaaToolkitAPI.h"
#include "MaaUtils/Encoding.h"
#include "MaaUtils/IOStream/BoostIO.hpp"
#include "MaaUtils/Platform.h"
#include <meojson/json.hpp>
#include <iostream>

int main()
{
    const std::string version = MaaVersion();
    if (version.empty() || MAA_NS::from_osstring(MAA_NS::to_osstring(version)) != version) {
        std::cerr << "failed to convert MaaVersion through MaaUtils\n";
        return 1;
    }

    auto value = json::parse(R"({ "package": "ok" })");
    if (!value || !value->as_object().contains("package") || value->as_object().at("package").as_string() != "ok") {
        std::cerr << "failed to parse meojson value\n";
        return 1;
    }

    return 0;
}
