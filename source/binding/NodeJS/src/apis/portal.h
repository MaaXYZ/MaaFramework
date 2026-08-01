#pragma once

#include <optional>
#include <string>

#include <MaaToolkit/MaaToolkitAPI.h>

#include "../foundation/spec.h"

struct PortalHelperImpl : public maajs::NativeClassBase
{
    MaaToolkitPortalHelper* helper { };
    bool own = false;

    PortalHelperImpl() = default;
    PortalHelperImpl(MaaToolkitPortalHelper* h, bool o);
    ~PortalHelperImpl();
    void destroy();

    maajs::PromiseType open_stream(maajs::EnvType env);
    void set_persist(bool enable);
    bool get_persist();
    int get_pipewire_fd();
    uint32_t get_pipewire_node_id();
    std::optional<std::string> get_restore_token();
    void set_restore_token(std::string token);

    std::string to_string() override;

    constexpr static char name[] = "PortalHelper";

    static PortalHelperImpl* ctor(const maajs::CallbackInfo&);
    static void init_proto(maajs::ObjectType proto, maajs::FunctionType ctor);
};
