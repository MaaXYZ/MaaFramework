module;

// https://github.com/scylladb/seastar/commit/131f637ac25583e79366ef716bf6812fd02bec6f
#include <compare>

#include <atomic>
#include <memory>
#include <variant>
#include <vector>

export module MaaPP:DynamicMerge;

import :Promise;
import :Utils;

#include "MaaPP/coro/details/DynamicMerge.hpp"
