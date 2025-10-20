#pragma once

#include <map>
#include <optional>

#include <MaaFramework/MaaAPI.h>

#include "../foundation/callback.h"
#include "../foundation/spec.h"

struct ResourceImpl : public maajs::NativeClassBase
{
    MaaResource* resource {};
    bool own = false;
    std::map<MaaSinkId, maajs::CallbackContext*> sinks {};

    ResourceImpl() = default;
    ResourceImpl(MaaResource* res, bool own);
    ~ResourceImpl();
    void destroy();
    MaaSinkId add_sink(maajs::FunctionType sink);
    void remove_sink(MaaSinkId id);
    void clear_sinks();
    maajs::ValueType post_bundle(maajs::ValueType self, maajs::EnvType env, std::string path);
    void override_pipeline(maajs::ValueType pipeline);
    void override_next(std::string node_name, std::vector<std::string> next_list);
    std::optional<std::string> get_node_data(std::string node_name);
    std::optional<maajs::ValueType> get_node_data_parsed(std::string node_name);
    void clear();
    MaaStatus status(MaaResId id);
    maajs::PromiseType wait(MaaResId id);
    bool get_loaded();
    std::optional<std::string> get_hash();
    std::optional<std::vector<std::string>> get_node_list();

    std::string to_string() override;

    static maajs::ValueType locate_object(maajs::EnvType env, MaaResource* res);

    constexpr static char name[] = "Resource";

    virtual void init_bind(maajs::ObjectType self) override;
    virtual void gc_mark(maajs::NativeMarkerFunc marker) override;
    static ResourceImpl* ctor(const maajs::CallbackInfo&);
    static void init_proto(maajs::ObjectType proto, maajs::FunctionType);
};

MAA_JS_NATIVE_CLASS_STATIC_FORWARD(ResourceImpl)
