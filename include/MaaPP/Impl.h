#pragma once

#include "./Context.h"
#include "./Tasker.h"

namespace maapp
{

inline std::shared_ptr<Tasker> Context::tasker() const
{
    return Tasker::find(MaaContextGetTasker(context_));
}

}
