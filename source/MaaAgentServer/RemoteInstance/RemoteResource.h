#pragma once

#include "Common/MaaTypes.h"
#include "MaaAgent/Transceiver.h"

MAA_AGENT_SERVER_NS_BEGIN

class RemoteResource : public MaaResource
{
public:
    RemoteResource(Transceiver& server, const std::string& resource_id);
    virtual ~RemoteResource() = default;

    virtual bool set_option(MaaResOption key, MaaOptionValue value, MaaOptionValueSize val_size) override;

    virtual MaaResId post_bundle(const std::filesystem::path& path) override;

    virtual MaaStatus status(MaaResId res_id) const override;
    virtual MaaStatus wait(MaaResId res_id) const override;
    virtual bool valid() const override;
    virtual bool running() const override;
    virtual bool clear() override;
    
    virtual bool override_pipeline(const json::object& pipeline_override) override;
    virtual bool override_next(const std::string& node_name, const std::vector<std::string>& next) override;
    virtual std::optional<json::object> get_node_data(const std::string& node_name) const override;

    virtual void register_custom_recognition(const std::string& name, MaaCustomRecognitionCallback recognition, void* trans_arg) override;
    virtual void unregister_custom_recognition(const std::string& name) override;
    virtual void clear_custom_recognition() override;
    virtual void register_custom_action(const std::string& name, MaaCustomActionCallback action, void* trans_arg) override;
    virtual void unregister_custom_action(const std::string& name) override;
    virtual void clear_custom_action() override;

    virtual std::string get_hash() const override;
    virtual std::vector<std::string> get_node_list() const override;

private:
    Transceiver& server_;
    std::string resource_id_;
};

MAA_AGENT_SERVER_NS_END
