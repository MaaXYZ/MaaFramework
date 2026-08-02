#include "portal.h"
#include "loader.h"

#include <MaaToolkit/MaaToolkitAPI.h>

#include "../foundation/spec.h"
#include "ext.h"

PortalHelperImpl::PortalHelperImpl(MaaToolkitPortalHelper* h, bool o)
    : helper(h)
    , own(o)
{
}

PortalHelperImpl::~PortalHelperImpl()
{
    destroy();
}

void PortalHelperImpl::destroy()
{
    if (!helper) {
        return;
    }

    if (own) {
        MaaToolkitPortalHelperDestroy(helper);
    }

    helper = nullptr;
    own = false;
}

maajs::PromiseType PortalHelperImpl::open_stream()
{
    using Result = bool;
    auto worker = new maajs::AsyncWork<Result>(env, [&]() -> Result {
        return MaaToolkitPortalHelperOpenStream(helper);
    });
    worker->Queue();
    return worker->Promise();
}

void PortalHelperImpl::set_persist(bool enable)
{
    MaaToolkitPortalHelperSetPersist(helper, enable);
}

bool PortalHelperImpl::get_persist()
{
    return MaaToolkitPortalHelperGetPersist(helper);
}

int PortalHelperImpl::get_pipewire_fd()
{
    return MaaToolkitPortalHelperGetPipeWireFD(helper);
}

uint32_t PortalHelperImpl::get_pipewire_node_id()
{
    return MaaToolkitPortalHelperGetPipeWireNodeID(helper);
}

std::optional<std::string> PortalHelperImpl::get_restore_token()
{
    auto token = MaaToolkitPortalHelperGetRestoreToken(helper);
    if (!token) {
        return std::nullopt;
    }
    return std::string(token);
}

void PortalHelperImpl::set_restore_token(std::string token)
{
    MaaToolkitPortalHelperSetRestoreToken(helper, token.c_str());
}

std::string PortalHelperImpl::to_string()
{
    return std::format(" handle = {:#018x}, {} ", reinterpret_cast<uintptr_t>(helper), own ? "owned" : "rented");
}

PortalHelperImpl* PortalHelperImpl::ctor(const maajs::CallbackInfo& info)
{
    std::ignore = info;
    auto h = MaaToolkitPortalHelperCreate();
    if (!h) {
        return nullptr;
    }
    return new PortalHelperImpl(h, true);
}

void PortalHelperImpl::init_proto(maajs::ObjectType proto, maajs::FunctionType)
{
    MAA_BIND_FUNC(proto, "destroy", PortalHelperImpl::destroy);
    MAA_BIND_FUNC(proto, "open_stream", PortalHelperImpl::open_stream);
    MAA_BIND_GETTER_SETTER(proto, "persist", PortalHelperImpl::get_persist, PortalHelperImpl::set_persist);
    MAA_BIND_GETTER(proto, "pipewire_fd", PortalHelperImpl::get_pipewire_fd);
    MAA_BIND_GETTER(proto, "pipewire_node_id", PortalHelperImpl::get_pipewire_node_id);
    MAA_BIND_GETTER_SETTER(proto, "restore_token", PortalHelperImpl::get_restore_token, PortalHelperImpl::set_restore_token);
}
