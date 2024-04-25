#pragma once

#include <memory>

#include "MaaPP/maa/Controller.hpp"
#include "MaaPP/maa/Instance.hpp"
#include "MaaPP/maa/Resource.hpp"

namespace maa
{

using ControllerHandle = std::shared_ptr<Controller>;
using ResourceHandle = std::shared_ptr<Resource>;
using InstanceHandle = std::shared_ptr<Instance>;

}
