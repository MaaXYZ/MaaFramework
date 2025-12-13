#pragma once

#include <mutex>
#include <optional>

#include <meojson/json.hpp>
#include <zmq.hpp>

#include "Common/MaaTypes.h"
#include "MaaUtils/Logger.h"
#include "Message.hpp"

#include "Common/Conf.h"

MAA_AGENT_NS_BEGIN

class Transceiver
{
    using ImageEncodedBuffer = std::vector<uint8_t>;

public:
    virtual ~Transceiver();

public:
    template <typename ResponseT, typename RequestT>
    std::optional<ResponseT> send_and_recv(const RequestT& req)
    {
        auto req_id = ++s_req_id_;

        // LogFunc << VAR(req_id);
        bool sent = send(req);
        if (!sent) {
            LogError << "failed to send req" << VAR(req_id);
            return std::nullopt;
        }

        for (int64_t loop_count = 0;; ++loop_count) {
            // LogTrace << "enter loop" << VAR(req_id) << VAR(loop_count);

            auto msg_opt = recv();
            if (!msg_opt) {
                LogError << "failed to recv resp" << VAR(req_id) << VAR(loop_count);
                return std::nullopt;
            }
            const json::value& msg = *msg_opt;
            if (msg.is<ResponseT>()) {
                // LogTrace << "response" << VAR(req_id) << VAR(loop_count);
                return msg.as<ResponseT>();
            }
            else if (msg.is<ImageHeader>()) {
                handle_image(msg.as<ImageHeader>());
            }
            else if (msg.is<ImageEncodedHeader>()) {
                handle_image_encoded(msg.as<ImageEncodedHeader>());
            }
            else {
                // LogTrace << "inserted request" << VAR(req_id) << VAR(loop_count);
                handle_inserted_request(msg);
            }
        }
        // unreachable code
        // return std::nullopt;
    }

    std::string send_image(const cv::Mat& mat);
    std::string send_image_encoded(const ImageEncodedBuffer& encoded_data);
    cv::Mat get_image_cache(const std::string& uuid);
    ImageEncodedBuffer get_image_encoded_cache(const std::string& uuid);

protected:
    virtual bool handle_inserted_request(const json::value& j) = 0;
    bool handle_image_header(const json::value& j);
    bool handle_image_encoded_header(const json::value& j);

    void init_socket(const std::string& identifier, bool bind);
    void uninit_socket();

    bool send(const json::value& j);
    std::optional<json::value> recv();

    bool alive();
    void set_timeout(const std::chrono::milliseconds& timeout);

private:
    void handle_image(const ImageHeader& header);
    void handle_image_encoded(const ImageEncodedHeader& header);
    bool poll(zmq::pollitem_t& pollitem);

protected:
    zmq::context_t zmq_ctx_;
    zmq::socket_t zmq_sock_;

    std::string ipc_addr_;
    std::filesystem::path ipc_path_;

    std::map<std::string /* uuid */, cv::Mat> recved_images_;
    std::map<std::string /* uuid */, ImageEncodedBuffer> recved_images_encoded_;

private:
    inline static int64_t s_req_id_ = 0;
    bool is_bound_ = false;

    std::mutex socket_mutex_;
    zmq::pollitem_t zmq_pollitem_send_ {};
    zmq::pollitem_t zmq_pollitem_recv_ {};
    std::chrono::milliseconds timeout_ = std::chrono::milliseconds::max();
};

MAA_AGENT_NS_END
