module;

#include <format>

module napi;

namespace fmt
{

template <typename... Args>
std::string format(std::string_view fmt, Args&&... args)
{
    return std::vformat(fmt, std::forward < Args >> (args)...);
}

}
