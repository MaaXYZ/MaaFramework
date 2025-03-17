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

}
