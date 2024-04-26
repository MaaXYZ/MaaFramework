#pragma once

#include <string>

#include <MaaFramework/MaaAPI.h>

namespace maa
{

struct AnalyzeResult
{
    bool result = false;
    MaaRect rec_box = { 0, 0, 0, 0 };
    std::string rec_detail = "";
};

}
