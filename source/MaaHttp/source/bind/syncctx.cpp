#include "../server.h"

#include "../spec/syncctx.hpp"

void bind_server_syncctx(lhg::server::Dispatcher*& dispatcher)
{
    lhg::server::apply_function<lhg::maa::__function_list_syncctx>(dispatcher);
}
