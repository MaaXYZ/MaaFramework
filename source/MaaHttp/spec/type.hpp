#pragma once

#include <memory>

#include "callback/callback.hpp"

namespace lhg
{

namespace pri_maa
{
    struct custom_action_context
    {
        std::shared_ptr<lhg::callback::context_info> run;
        std::shared_ptr<lhg::callback::context_info> stop;
    };
}

}