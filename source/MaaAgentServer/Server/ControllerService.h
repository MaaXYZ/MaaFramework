#pragma once

#include <thread>

#include "MaaAgent/Transceiver.h"
#include "MaaFramework/Instance/MaaCustomController.h"

MAA_AGENT_SERVER_NS_BEGIN

inline thread_local bool is_agent_server_thread = false;

struct ControllerRegistration
{
    MaaCustomControllerCallbacks callbacks;
    void* arg = nullptr;
};

class ControllerService : public Transceiver
{
public:
    explicit ControllerService(std::map<std::string, ControllerRegistration> registrations);
    ~ControllerService() override;

    std::string start(bool tcp);

private:
    void run();
    bool handle_inserted_request(const json::value& j) override;
    CustomControllerResponse invoke(const ControllerRegistration& registration, const CustomControllerRequest& req);

    std::map<std::string, ControllerRegistration> registrations_;
    std::thread thread_;
};

MAA_AGENT_SERVER_NS_END
