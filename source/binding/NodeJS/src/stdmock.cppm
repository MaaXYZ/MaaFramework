module;

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <exception>
#include <format>
#include <functional>
#include <future>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

export module stdmock;

export using ::size_t;

export namespace std
{

using std::exception;
using std::function;
using std::future;
using std::index_sequence;
using std::int32_t;
using std::int64_t;
using std::map;
using std::optional;
using std::ostream_iterator;
using std::ostringstream;
using std::promise;
using std::string;
using std::string_view;
using std::tuple;
using std::tuple_element_t;
using std::uint32_t;
using std::uint64_t;
using std::unique_ptr;
using std::vector;

using std::apply;
using std::begin;
using std::cerr;
using std::copy;
using std::declval;
using std::end;
using std::endl;
using std::format;
using std::get;
using std::ignore;
using std::is_same_v;
using std::make_index_sequence;
using std::make_shared;
using std::make_tuple;
using std::memcpy;
using std::move;
using std::nullopt;
using std::stoll;
using std::stoull;
using std::strtoull;
using std::to_string;
using std::tuple_cat;
using std::tuple_size_v;

using std::operator==;
using std::operator!=;
using std::operator<;
using std::operator>;
using std::operator<=;
using std::operator>=;
using std::operator<=>;
using std::operator-;
using std::operator+;

}

// libstdc++的迭代器 operator== 在命名空间里面
// https://github.com/alibaba/async_simple/blob/main/async_simple/std.mock.cppm
// https://github.com/MaaXYZ/MaaFramework/actions/runs/13918756240/job/38946866471
#if defined(__GLIBCXX__) || defined(__GLIBCPP__)
export namespace __gnu_cxx
{
using __gnu_cxx::operator==;
using __gnu_cxx::operator-;
} // namespace __gnu_cxx
#endif

#if defined(_WIN32)
// 不知道为啥, 但是这里需要提供一个同名的模板函数, 否则 msvc 下有语法错误, 虽然根本没用这个函数
// https://github.com/MaaXYZ/MaaFramework/actions/runs/13922021657/job/38957311261
// https://github.com/MaaXYZ/MaaFramework/actions/runs/13922189281/job/38957838620
template <size_t N>
void make_index_sequence();
#endif
