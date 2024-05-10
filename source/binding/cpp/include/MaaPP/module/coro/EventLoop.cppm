module;

#include <chrono>
#include <functional>
#include <thread>

export module MaaPP:EventLoop;

import :Promise;
import :ThreadPool;

#include "MaaPP/coro/EventLoop.hpp"
