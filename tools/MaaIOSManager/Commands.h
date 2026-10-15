#pragma once

#include "Cli.h"

namespace maa::ios::tool
{
int execute(const Options& options, std::ostream& output, std::ostream& errors, const std::function<bool()>& cancelled);
int prepare(const Options& options, std::ostream& output, std::ostream& errors, const std::function<bool()>& cancelled);
}
