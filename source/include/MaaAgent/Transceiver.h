#pragma once

#include <optional>

#include <meojson/json.hpp>

#include "Common/MaaTypes.h"
#include "Utils/Logger.h"

MAA_AGENT_NS_BEGIN

class Transceiver
{
public:
    virtual ~Transceiver() = default;

public:
    template <typename ResponseT, typename RequestT>
    std::optional<ResponseT> send_and_recv(const RequestT& req)
    {
        auto req_id = ++s_req_id_;

        LogFunc << VAR(req_id);
        bool sent = send(req);
        if (!sent) {
            LogError << "failed to send req" << VAR(req_id);
            return std::nullopt;
        }

        for (int64_t loop_count = 0;; ++loop_count) {
            LogTrace << "enter loop" << VAR(req_id) << VAR(loop_count);

            auto msg_opt = recv();
            if (!msg_opt) {
                LogError << "failed to recv resp" << VAR(req_id) << VAR(loop_count);
                return std::nullopt;
            }
            const json::value& msg = *msg_opt;
            if (msg.is<ResponseT>()) {
                LogTrace << "response" << VAR(req_id) << VAR(loop_count);
                return msg.as<ResponseT>();
            }
            else {
                LogTrace << "inserted request" << VAR(req_id) << VAR(loop_count);
                handle_inserted_request(msg);
            }
        }
        // unreachable code
        // return std::nullopt;
    }

protected:
    virtual bool send(const json::value& j) = 0;
    virtual std::optional<json::value> recv() = 0;
    virtual bool handle_inserted_request(const json::value& j) = 0;

private:
    inline static int64_t s_req_id_ = 0;
};

MAA_AGENT_NS_END
