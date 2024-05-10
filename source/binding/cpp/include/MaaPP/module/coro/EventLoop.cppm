module;

#include <chrono>
#include <thread>

export module MaaPP:EventLoop;

import :Promise;
import :ThreadPool;

#include "MaaPP/coro/EventLoop.hpp"
