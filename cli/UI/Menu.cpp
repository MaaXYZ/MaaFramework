#include "UI/Menu.h"

#include <format>
#include <iostream>

namespace maa::cli::ui
{

coro::Promise<long> input_number()
{
    return coro::EventLoop::current()->eval([]() {
        char ch;
        std::cin >> ch;
        std::cin.putback(ch);

        std::string row;
        std::getline(std::cin, row);

        try {
            return std::stol(row);
        }
        catch (...) {
            return -1l;
        }
    });
}

coro::Promise<> menu(std::string title, std::vector<MenuEntry> entries)
{
    std::cout << title << std::endl;
    std::cout << std::endl;
    for (size_t i = 0; i < entries.size(); i++) {
        const auto& entry = entries[i];
        if (auto* text = std::get_if<0>(&entry.render)) {
            std::cout << '\t' << (i + 1) << ". " << *text << std::endl;
        }
        else if (auto* render = std::get_if<1>(&entry.render)) {
            (*render)(i);
        }
    }
    std::cout << std::endl;
    std::cout << std::format("Please input [{}-{}]: ", 1, entries.size()) << std::flush;
    while (true) {
        auto choice = static_cast<size_t>(co_await input_number());
        if (choice == 0 || choice > entries.size()) {
            std::cout
                << std::format("Choice out of range, please reinput [{}, {}]: ", 1, entries.size())
                << std::flush;
        }
        else {
            entries[choice - 1].action(choice);
            co_return;
        }
    }
}

}
