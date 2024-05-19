#include "../server.h"

#include "../spec/toolkit.hpp"

void bind_server_toolkit(lhg::server::Dispatcher*& dispatcher)
{
    lhg::server::apply_function<lhg::maa::__function_list_toolkit>(dispatcher);
}
