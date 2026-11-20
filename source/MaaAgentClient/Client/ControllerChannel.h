#pragma once

#include <memory>

#include "MaaAgent/Transceiver.h"

MAA_AGENT_CLIENT_NS_BEGIN

class ControllerChannel
    : public Transceiver
    , public std::enable_shared_from_this<ControllerChannel>
{
public:
    static std::shared_ptr<ControllerChannel> open(std::string_view endpoint, bool tcp, const std::vector<std::string>& names);

    using Transceiver::set_timeout;

    const std::vector<std::string>& names() const { return names_; }

    MaaController* create(const std::string& name);
    void close();

    CustomControllerResponse invoke(const CustomControllerRequest& req, MaaImageBuffer* image);

private:
    bool handle_inserted_request(const json::value&) override { return false; }

    std::vector<std::string> names_;
    std::mutex call_mutex_;
    std::atomic_bool closed_ = false;
};

MAA_AGENT_CLIENT_NS_END
