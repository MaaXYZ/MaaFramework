#include "../server.h"

#include "../spec/buffer.hpp"

void bind_server_buffer(lhg::server::Dispatcher*& dispatcher)
{
    lhg::server::apply_function<lhg::maa::__function_list_buffer>(dispatcher);
}
