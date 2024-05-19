#include "../server.h"

#include "../spec/rest.hpp"

void bind_server_misc(lhg::server::Dispatcher*& dispatcher)
{
    lhg::server::apply_callback<lhg::maa::__callback_list>(dispatcher);
    lhg::server::apply_handle<lhg::maa::__handle_list>(dispatcher);
}
