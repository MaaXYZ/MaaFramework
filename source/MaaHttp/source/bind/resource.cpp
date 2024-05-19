#include "../server.h"

#include "../spec/resource.hpp"

void bind_server_resource(lhg::server::Dispatcher*& dispatcher)
{
    lhg::server::apply_function<lhg::maa::__function_list_resource>(dispatcher);
}
