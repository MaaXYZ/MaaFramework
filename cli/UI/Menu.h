#pragma once

#include <functional>
#include <string>
#include <variant>
#include <vector>

#include <MaaPP/MaaPP.hpp>

namespace maa::cli::ui
{

struct MenuEntry
{
    std::variant<std::string, std::function<void(size_t entry)>> render;
    std::function<void(size_t entry)> action;
};

coro::Promise<long> input_number();
coro::Promise<> menu(std::string title, std::vector<MenuEntry> entries);

}
