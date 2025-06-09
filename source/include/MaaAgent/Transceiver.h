#pragma once

#include <optional>

#include <meojson/json.hpp>
#include <zmq.hpp>

#include "Common/MaaTypes.h"
#include "Message.hpp"
#include "Utils/Logger.h"

#include "timers.hpp"

MAA_AGENT_NS_BEGIN

class Transceiver
{
public:
    virtual ~Transceiver();

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
            else if (msg.is<ImageHeader>()) {
                handle_image(msg.as<ImageHeader>());
            }
            else {
                LogTrace << "inserted request" << VAR(req_id) << VAR(loop_count);
                handle_inserted_request(msg);
            }
        }
        // unreachable code
        // return std::nullopt;
    }

    std::string send_image(const cv::Mat& mat);
    cv::Mat get_image_cache(const std::string& uuid);

protected:
    virtual bool handle_inserted_request(const json::value& j) = 0;
    bool handle_image_header(const json::value& j);

    void init_socket(const std::string& identifier, bool bind);
    void uninit_socket();

    bool send(const json::value& j);
    std::optional<json::value> recv();

    bool alive();
    void set_timeout(std::chrono::milliseconds timeout);

private:
    void handle_image(const ImageHeader& header);
    bool poll(zmq::pollitem_t* pollitem);

    static void timeout_callback(int timer_id, void* trans_arg)
    {
        auto* pthis = reinterpret_cast<Transceiver*>(trans_arg);
        LogWarn << VAR(timer_id) << "server is not alive" << VAR(pthis->ipc_addr_);
        pthis->need_to_cancel_ = true;
    }

protected:
    zmq::socket_t zmq_sock_;
    zmq::context_t zmq_ctx_;

    std::string ipc_addr_;

    std::map<std::string /* uuid */, cv::Mat> recved_images_;

private:
    inline static int64_t s_req_id_ = 0;
    bool is_bound_ = false;

    bool need_to_cancel_ = false;
    zmq::pollitem_t zmq_pollitems_[2];
    zmq::timers zmq_timers_ = zmq::timers();
};

MAA_AGENT_NS_END
