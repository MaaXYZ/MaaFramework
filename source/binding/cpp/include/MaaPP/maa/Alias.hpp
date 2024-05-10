// IWYU pragma: private, include <MaaPP/MaaPP.hpp>

#pragma once

#include "MaaPP/module/Module.h"

#ifndef MAAPP_USE_MODULE

#include <memory>

#include "MaaPP/maa/Controller.hpp"
#include "MaaPP/maa/Image.hpp"
#include "MaaPP/maa/Instance.hpp"
#include "MaaPP/maa/Resource.hpp"
#include "MaaPP/maa/SyncContext.hpp"

#endif

namespace maa
{

MAAPP_EXPORT using ControllerHandle = std::shared_ptr<Controller>;
MAAPP_EXPORT using ResourceHandle = std::shared_ptr<Resource>;
MAAPP_EXPORT using InstanceHandle = std::shared_ptr<Instance>;
MAAPP_EXPORT using SyncContextHandle = std::shared_ptr<SyncContext>;
MAAPP_EXPORT using ImageHandle = std::shared_ptr<details::Image>;

}
