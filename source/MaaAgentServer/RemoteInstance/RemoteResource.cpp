#include "RemoteResource.h"

#include "MaaAgent/Message.hpp"
#include "MaaUtils/Platform.h"

MAA_AGENT_SERVER_NS_BEGIN

RemoteResource::RemoteResource(Transceiver& server, const std::string& resource_id)
    : server_(server)
    , resource_id_(resource_id)
{
}

bool RemoteResource::set_option(MaaResOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogError << "Can NOT set option at remote resource" << VAR(key) << VAR_VOIDP(value) << VAR(val_size);
    return false;
}

MaaResId RemoteResource::post_bundle(const std::filesystem::path& path)
{
    ResourcePostBundleReverseRequest req {
        .resource_id = resource_id_,
        .path = path_to_utf8_string(path),
    };
    auto resp_opt = server_.send_and_recv<ResourcePostBundleReverseResponse>(req);
    if (!resp_opt) {
        return MaaInvalidId;
    }
    return resp_opt->res_id;
}

MaaResId RemoteResource::post_ocr_model(const std::filesystem::path& path)
{
    ResourcePostOcrModelReverseRequest req {
        .resource_id = resource_id_,
        .path = path_to_utf8_string(path),
    };
    auto resp_opt = server_.send_and_recv<ResourcePostOcrModelReverseResponse>(req);
    if (!resp_opt) {
        return MaaInvalidId;
    }
    return resp_opt->res_id;
}

MaaResId RemoteResource::post_pipeline(const std::filesystem::path& path)
{
    ResourcePostPipelineReverseRequest req {
        .resource_id = resource_id_,
        .path = path_to_utf8_string(path),
    };
    auto resp_opt = server_.send_and_recv<ResourcePostPipelineReverseResponse>(req);
    if (!resp_opt) {
        return MaaInvalidId;
    }
    return resp_opt->res_id;
}

MaaResId RemoteResource::post_image(const std::filesystem::path& path)
{
    ResourcePostImageReverseRequest req {
        .resource_id = resource_id_,
        .path = path_to_utf8_string(path),
    };
    auto resp_opt = server_.send_and_recv<ResourcePostImageReverseResponse>(req);
    if (!resp_opt) {
        return MaaInvalidId;
    }
    return resp_opt->res_id;
}

MaaStatus RemoteResource::status(MaaResId res_id) const
{
    ResourceStatusReverseRequest req {
        .resource_id = resource_id_,
        .res_id = res_id,
    };
    auto resp_opt = server_.send_and_recv<ResourceStatusReverseResponse>(req);
    if (!resp_opt) {
        return MaaStatus_Invalid;
    }
    return static_cast<MaaStatus>(resp_opt->status);
}

MaaStatus RemoteResource::wait(MaaResId res_id) const
{
    ResourceWaitReverseRequest req {
        .resource_id = resource_id_,
        .res_id = res_id,
    };
    auto resp_opt = server_.send_and_recv<ResourceWaitReverseResponse>(req);
    if (!resp_opt) {
        return MaaStatus_Invalid;
    }
    return static_cast<MaaStatus>(resp_opt->status);
}

bool RemoteResource::valid() const
{
    ResourceValidReverseRequest req {
        .resource_id = resource_id_,
    };

    auto resp_opt = server_.send_and_recv<ResourceValidReverseResponse>(req);
    if (!resp_opt) {
        return false;
    }

    return resp_opt->ret;
}

bool RemoteResource::running() const
{
    ResourceRunningReverseRequest req {
        .resource_id = resource_id_,
    };

    auto resp_opt = server_.send_and_recv<ResourceRunningReverseResponse>(req);
    if (!resp_opt) {
        return false;
    }

    return resp_opt->ret;
}

bool RemoteResource::clear()
{
    ResourceClearReverseRequest req {
        .resource_id = resource_id_,
    };
    auto resp_opt = server_.send_and_recv<ResourceClearReverseResponse>(req);
    if (!resp_opt) {
        return false;
    }
    return resp_opt->ret;
}

bool RemoteResource::override_pipeline(const json::value& pipeline_override)
{
    ResourceOverridePipelineReverseRequest req {
        .resource_id = resource_id_,
        .pipeline_override = pipeline_override,
    };
    auto resp_opt = server_.send_and_recv<ResourceOverridePipelineReverseResponse>(req);
    if (!resp_opt) {
        return false;
    }
    return resp_opt->ret;
}

bool RemoteResource::override_next(const std::string& node_name, const std::vector<std::string>& next)
{
    ResourceOverrideNextReverseRequest req {
        .resource_id = resource_id_,
        .node_name = node_name,
        .next = next,
    };
    auto resp_opt = server_.send_and_recv<ResourceOverrideNextReverseResponse>(req);
    if (!resp_opt) {
        return false;
    }
    return resp_opt->ret;
}

bool RemoteResource::override_image(const std::string& image_name, const cv::Mat& image)
{
    ResourceOverrideImageReverseRequest req {
        .resource_id = resource_id_,
        .image_name = image_name,
        .image = server_.send_image(image),
    };

    auto resp_opt = server_.send_and_recv<ResourceOverrideImageReverseResponse>(req);
    if (!resp_opt) {
        return false;
    }
    return resp_opt->ret;
}

std::optional<json::object> RemoteResource::get_node_data(const std::string& node_name) const
{
    ResourceGetNodeDataReverseRequest req {
        .resource_id = resource_id_,
        .node_name = node_name,
    };

    auto resp_opt = server_.send_and_recv<ResourceGetNodeDataReverseResponse>(req);
    if (!resp_opt || !resp_opt->has_value) {
        return std::nullopt;
    }

    return resp_opt->node_data;
}

void RemoteResource::register_custom_recognition(const std::string& name, MaaCustomRecognitionCallback recognition, void* trans_arg)
{
    LogError << "Can NOT register custom recognition at remote resource" << VAR(name) << VAR_VOIDP(recognition) << VAR_VOIDP(trans_arg);
}

void RemoteResource::unregister_custom_recognition(const std::string& name)
{
    LogError << "Can NOT unregister custom recognition at remote resource" << VAR(name);
}

void RemoteResource::clear_custom_recognition()
{
    LogError << "Can NOT clear custom recognition at remote resource";
}

void RemoteResource::register_custom_action(const std::string& name, MaaCustomActionCallback action, void* trans_arg)
{
    LogError << "Can NOT register custom action at remote resource" << VAR(name) << VAR_VOIDP(action) << VAR_VOIDP(trans_arg);
}

void RemoteResource::unregister_custom_action(const std::string& name)
{
    LogError << "Can NOT unregister custom action at remote resource" << VAR(name);
}

void RemoteResource::clear_custom_action()
{
    LogError << "Can NOT clear custom action at remote resource";
}

std::string RemoteResource::get_hash() const
{
    ResourceGetHashReverseRequest req {
        .resource_id = resource_id_,
    };
    auto resp_opt = server_.send_and_recv<ResourceGetHashReverseResponse>(req);
    if (!resp_opt) {
        return {};
    }
    return resp_opt->hash;
}

std::vector<std::string> RemoteResource::get_node_list() const
{
    ResourceGetNodeListReverseRequest req {
        .resource_id = resource_id_,
    };
    auto resp_opt = server_.send_and_recv<ResourceGetNodeListReverseResponse>(req);
    if (!resp_opt) {
        return {};
    }
    return resp_opt->node_list;
}

std::vector<std::string> RemoteResource::get_custom_recognition_list() const
{
    ResourceGetCustomRecognitionListReverseRequest req {
        .resource_id = resource_id_,
    };
    auto resp_opt = server_.send_and_recv<ResourceGetCustomRecognitionListReverseResponse>(req);
    if (!resp_opt) {
        return {};
    }
    return resp_opt->custom_recognition_list;
}

std::vector<std::string> RemoteResource::get_custom_action_list() const
{
    ResourceGetCustomActionListReverseRequest req {
        .resource_id = resource_id_,
    };
    auto resp_opt = server_.send_and_recv<ResourceGetCustomActionListReverseResponse>(req);
    if (!resp_opt) {
        return {};
    }
    return resp_opt->custom_action_list;
}

MaaSinkId RemoteResource::add_sink(MaaEventCallback callback, void* trans_arg)
{
    LogError << "Can NOT add sink for remote instance, use AgentServer.add_resource_sink instead" << VAR_VOIDP(callback)
             << VAR_VOIDP(trans_arg);
    return MaaInvalidId;
}

void RemoteResource::remove_sink(MaaSinkId sink_id)
{
    LogError << "Can NOT remove sink for remote instance" << VAR(sink_id);
}

void RemoteResource::clear_sinks()
{
    LogError << "Can NOT clear sink for remote instance";
}

MAA_AGENT_SERVER_NS_END
