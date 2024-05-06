// IWYU pragma: private, include <MaaPP/MaaPP.hpp>

#pragma once

#include <memory>

#include "MaaPP/maa/Controller.hpp"
#include "MaaPP/maa/Instance.hpp"
#include "MaaPP/maa/Resource.hpp"
#include "MaaPP/maa/SyncContext.hpp"

namespace maa
{

using ControllerHandle = std::shared_ptr<Controller>;
using ResourceHandle = std::shared_ptr<Resource>;
using InstanceHandle = std::shared_ptr<Instance>;
using SyncContextHandle = std::shared_ptr<SyncContext>;
using ImageHandle = std::shared_ptr<details::Image>;

}
