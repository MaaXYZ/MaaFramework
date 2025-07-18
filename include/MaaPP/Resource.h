#pragma once

#include <functional>
#include <map>
#include <memory>
#include <tuple>
#include <vector>

#include <MaaFramework/MaaAPI.h>

#include "./Context.h"
#include "./Custom.h"
#include "./Exception.h"
#include "./Reco.h"
#include "./Task.h"
#include "./private/String.h"

namespace maapp
{

struct Resource : public std::enable_shared_from_this<Resource>
{
    Resource()
    {
        resource_ = MaaResourceCreate(
            +[](const char* message, const char* details_json, void* notify_trans_arg) {
                auto self = static_cast<Resource*>(notify_trans_arg)->shared_from_this();

                std::ignore = message;
                std::ignore = details_json;
            },
            this);

        resource_map_[resource_] = weak_from_this();
    }

    Resource(const Resource&) = delete;

    virtual ~Resource()
    {
        resource_map_.erase(resource_);
        MaaResourceDestroy(resource_);
    }

    Resource& operator=(const Resource&) = delete;

    template <typename Func>
    void register_custom_recognition(const std::string& name, Func&& func)
    {
        if (!MaaResourceRegisterCustomRecognition(
                resource_,
                name.c_str(),
                &pri::custom_reco_wrapper<
                    std::shared_ptr<Resource>,
                    +[](void* arg, const char* name) {
                        auto self = reinterpret_cast<Resource*>(arg)->shared_from_this();
                        const auto& func = self->recos_[name];
                        return std::make_tuple(func, self);
                    }>,
                this)) {
            throw FunctionFailed("MaaResourceRegisterCustomRecognition");
        }
        recos_[name] = func;
    }

    void unregister_custom_recognition(const std::string& name)
    {
        if (!MaaResourceUnregisterCustomRecognition(resource_, name.c_str())) {
            throw FunctionFailed("MaaResourceUnregisterCustomRecognition");
        }
        recos_.erase(name);
    }

    void clear_custom_recognition()
    {
        if (!MaaResourceClearCustomRecognition(resource_)) {
            throw FunctionFailed("MaaResourceClearCustomRecognition");
        }
        recos_.clear();
    }

    template <typename Func>
    void register_custom_action(const std::string& name, Func&& func)
    {
        if (!MaaResourceRegisterCustomAction(
                resource_,
                name.c_str(),
                &pri::custom_act_wrapper<
                    std::shared_ptr<Resource>,
                    +[](void* arg, const char* name) {
                        auto self = reinterpret_cast<Resource*>(arg)->shared_from_this();
                        const auto& func = self->actions_[name];
                        return std::make_tuple(func, self);
                    }>,
                this)) {
            throw FunctionFailed("MaaResourceRegisterCustomAction");
        }
        actions_[name] = func;
    }

    void unregister_custom_action(const std::string& name)
    {
        if (!MaaResourceUnregisterCustomAction(resource_, name.c_str())) {
            throw FunctionFailed("MaaResourceUnregisterCustomAction");
        }
        actions_.erase(name);
    }

    void clear_custom_action()
    {
        if (!MaaResourceClearCustomAction(resource_)) {
            throw FunctionFailed("MaaResourceClearCustomAction");
        }
        actions_.clear();
    }

    ResTask post_bundle(const std::string& path)
    {
        return {
            resource_,
            MaaResourcePostBundle(resource_, path.c_str()),
        };
    }

    void override_pipeline(const std::string& pipeline_override)
    {
        if (!MaaResourceOverridePipeline(resource_, pipeline_override.c_str())) {
            throw FunctionFailed("MaaResourceOverridePipeline");
        }
    }

    void override_next(const std::string& node_name, const std::vector<std::string>& next_list)
    {
        pri::StringList nexts;
        nexts = next_list;
        if (!MaaResourceOverrideNext(resource_, node_name.c_str(), nexts.buffer_)) {
            throw FunctionFailed("MaaResourceOverrideNext");
        }
    }

    std::string get_node_data(const std::string& node_name) const
    {
        pri::String node;
        if (!MaaResourceGetNodeData(resource_, node_name.c_str(), node.buffer_)) {
            throw FunctionFailed("MaaResourceGetNodeData");
        }
        return node;
    }

    void clear()
    {
        if (!MaaResourceClear(resource_)) {
            throw FunctionFailed("MaaResourceClear");
        }
    }

    bool loaded() const { return MaaResourceLoaded(resource_); }

    std::string hash() const
    {
        pri::String hash;
        if (!MaaResourceGetHash(resource_, hash.buffer_)) {
            throw FunctionFailed("MaaResourceGetHash");
        }
        return hash;
    }

    std::vector<std::string> node_list() const
    {
        pri::StringList node_list;
        if (!MaaResourceGetNodeList(resource_, node_list.buffer_)) {
            throw FunctionFailed("MaaResourceGetNodeList");
        }
        return node_list;
    }

    void set_inference_device(MaaInferenceDevice id)
    {
        if (!MaaResourceSetOption(resource_, MaaResOption_InferenceDevice, &id, sizeof(id))) {
            throw FunctionFailed("MaaResourceSetOption");
        }
    }

    void set_inference_device_cpu() { set_inference_device(MaaInferenceDevice_CPU); }

    void set_inference_device_auto() { set_inference_device(MaaInferenceDevice_Auto); }

    void set_inference_execution_provider(MaaInferenceExecutionProvider iep)
    {
        if (!MaaResourceSetOption(resource_, MaaResOption_InferenceExecutionProvider, &iep, sizeof(iep))) {
            throw FunctionFailed("MaaResourceSetOption");
        }
    }

    void set_inference_execution_provider_auto() { set_inference_execution_provider(MaaInferenceExecutionProvider_Auto); }

    void set_inference_execution_provider_cpu() { set_inference_execution_provider(MaaInferenceExecutionProvider_CPU); }

    void set_inference_execution_provider_direct_ml() { set_inference_execution_provider(MaaInferenceExecutionProvider_DirectML); }

    void set_inference_execution_provider_core_ml() { set_inference_execution_provider(MaaInferenceExecutionProvider_CoreML); }

    void set_inference_execution_provider_cuda() { set_inference_execution_provider(MaaInferenceExecutionProvider_CUDA); }

    MaaResource* resource_ {};
    std::map<std::string, CustomRecognition> recos_;
    std::map<std::string, CustomAction> actions_;

    static std::shared_ptr<Resource> find(MaaResource* resource)
    {
        auto it = resource_map_.find(resource);
        if (it == resource_map_.end()) {
            return nullptr;
        }
        return it->second.lock();
    }

    static inline std::map<MaaResource*, std::weak_ptr<Resource>> resource_map_ {};
};

}
