#pragma once

#ifdef MAA_USE_FMTLIB

#define FMT_HEADER_ONLY
#include <fmt/format.h>

#else

#include <format>
namespace fmt = std;

#endif
