#include "../server.h"

#include "../spec/rest.hpp"

void bind_server_rest(lhg::server::Dispatcher*& dispatcher)
{
    lhg::server::apply_function<lhg::maa::__function_list_>(dispatcher);
}
