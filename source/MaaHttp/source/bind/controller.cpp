#include "../server.h"

#include "../spec/controller.hpp"

void bind_server_controller(lhg::server::Dispatcher*& dispatcher)
{
    lhg::server::apply_function<lhg::maa::__function_list_controller>(dispatcher);
}
