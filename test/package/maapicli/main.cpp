#include "ProjectInterface/Parser.h"
#include <meojson/json.hpp>
#include <iostream>

int main()
{
    auto value = json::parse("{}");
    if (!value) {
        std::cerr << "failed to parse test JSON\n";
        return 1;
    }

    if (MAA_PROJECT_INTERFACE_NS::Parser::parse_interface(*value)) {
        std::cerr << "an empty object must not be a valid interface\n";
        return 1;
    }

    return 0;
}
