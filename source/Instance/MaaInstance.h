#pragma once

#include <condition_variable>
#include <future>
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include "Common/MaaMsg.h"
#include "Common/MaaTypes.h"

MAA_NS_BEGIN

class MaaInstance : public MaaInstanceAPI
{
public:
    virtual ~MaaInstance() override;

    virtual MaaInstanceAPI* create_ex(const std::filesystem::path& user_path, MaaInstanceCallback callback,
                                      void* callback_arg) override;
    virtual void destroy(MaaInstanceAPI** handle_ptr) override;

    virtual bool bind_resource(MaaResourceAPI* resource) override;
    virtual bool bind_controller(MaaControllerAPI* controller) override;
    virtual bool inited() const override;

    virtual bool set_option(InstanceOptionKey key, const std::string& value) override;

    virtual MaaTaskId append_task(const std::string& type, const std::string& param) override;
    virtual bool set_task_param(MaaTaskId task_id, const std::string& param) override;
    virtual std::vector<MaaTaskId> get_task_list() const override;

    virtual bool start() override;
    virtual bool stop() override;
    virtual bool running() const override;

    virtual std::string get_resource_hash() const override;
    virtual std::string get_controller_uuid() const override;

protected:
    MaaInstance(const std::filesystem::path& user_path, MaaInstanceCallback callback, void* callback_arg);

    std::filesystem::path user_path_;
    MaaInstanceCallback callback_ = nullptr;
    void* callback_arg_ = nullptr;

    MaaResourceAPI* resource_;
    MaaControllerAPI* controller_;
};

MAA_NS_END
