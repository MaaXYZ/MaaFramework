#include "Cli.h"
#include "Commands.h"

#include <atomic>
#include <csignal>
#include <exception>
#include <iostream>

namespace
{
std::atomic_bool interrupted { false };
static_assert(std::atomic_bool::is_always_lock_free);

void on_signal(int)
{
    interrupted.store(true, std::memory_order_relaxed);
}
}

int main(int argc, char** argv)
{
    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);
    std::vector<std::string_view> arguments;
    for (int index = 1; index < argc; ++index) {
        arguments.emplace_back(argv[index]);
    }
    maa::ios::tool::Options options;
    std::string error;
    if (!maa::ios::tool::parse(arguments, options, error)) {
        std::cerr << error << "\nRun MaaIOSManager --help for usage.\n";
        return 2;
    }
    try {
        return maa::ios::tool::run(
            options,
            std::cout,
            std::cerr,
            [] { return interrupted.load(std::memory_order_relaxed); },
            maa::ios::tool::execute);
    }
    catch (const std::exception& exception) {
        std::cerr << "Device operation failed: " << maa::ios::tool::terminal_text(exception.what()) << '\n';
        return interrupted.load(std::memory_order_relaxed) ? 130 : 1;
    }
}
