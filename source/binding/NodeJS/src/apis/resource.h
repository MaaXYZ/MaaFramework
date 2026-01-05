#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include <MaaFramework/MaaAPI.h>

#include "../foundation/spec.h"

struct ResourceImpl : public maajs::NativeClassBase
{
    MaaResource* resource {};
    bool own = false;
    std::map<MaaSinkId, maajs::CallbackContext*> sinks {};
    std::map<std::string, maajs::CallbackContext*> recos {};
    std::map<std::string, maajs::CallbackContext*> acts {};

    ResourceImpl() = default;
    ResourceImpl(MaaResource* res, bool own);
    ~ResourceImpl();
    void destroy();
    MaaSinkId add_sink(maajs::FunctionType sink);
    void remove_sink(MaaSinkId id);
    void clear_sinks();
    void set_inference_device(std::variant<std::string, int32_t> id);
    void set_inference_execution_provider(std::string provider);
    void register_custom_recognition(std::string name, maajs::FunctionType func);
    void unregister_custom_recognition(std::string name);
    void clear_custom_recognition();
    void register_custom_action(std::string name, maajs::FunctionType func);
    void unregister_custom_action(std::string name);
    void clear_custom_action();
    maajs::ValueType post_bundle(maajs::ValueType self, maajs::EnvType env, std::string path);
    maajs::ValueType post_ocr_model(maajs::ValueType self, maajs::EnvType env, std::string path);
    maajs::ValueType post_pipeline(maajs::ValueType self, maajs::EnvType env, std::string path);
    maajs::ValueType post_image(maajs::ValueType self, maajs::EnvType env, std::string path);
    void override_pipeline(maajs::ValueType pipeline);
    void override_next(std::string node_name, std::vector<std::string> next_list);
    void override_image(std::string image_name, maajs::ArrayBufferType image);
    std::optional<std::string> get_node_data(std::string node_name);
    std::optional<maajs::ValueType> get_node_data_parsed(std::string node_name);
    void clear();
    MaaStatus status(MaaResId id);
    maajs::PromiseType wait(MaaResId id);
    bool get_loaded();
    std::optional<std::string> get_hash();
    std::optional<std::vector<std::string>> get_node_list();
    std::optional<std::vector<std::string>> get_custom_recognition_list();
    std::optional<std::vector<std::string>> get_custom_action_list();

    std::string to_string() override;

    static maajs::ValueType locate_object(maajs::EnvType env, MaaResource* res);

    constexpr static char name[] = "Resource";

    virtual void init_bind(maajs::ObjectType self) override;
    virtual void gc_mark(maajs::NativeMarkerFunc marker) override;
    static ResourceImpl* ctor(const maajs::CallbackInfo&);
    static void init_proto(maajs::ObjectType proto, maajs::FunctionType);
};

