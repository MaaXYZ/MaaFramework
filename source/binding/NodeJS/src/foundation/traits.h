#pragma once

#include <MaaFramework/MaaAPI.h>

namespace maa_js
{

template <typename Type>
struct ExternalTraits
{
    constexpr static auto name = Type::name;
};

template <>
struct ExternalTraits<MaaContext>
{
    constexpr static auto name = "Context";
};

}
