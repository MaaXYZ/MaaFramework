#pragma once

#include <atomic>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <vector>

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

        // 等回包期间登记为活跃；无论怎样离开（含异常）都注销，迟到的回包据此丢弃
        begin_request(req_id);

        struct EndGuard
        {
            Transceiver* self;
            int64_t req_id;

            ~EndGuard() { self->end_request(req_id); }
        } end_guard { this, req_id };

        // LogFunc << VAR(req_id);
        json::value req_json = req;
        req_json[kReqIdKey] = req_id;
        bool sent = send(req_json);
        if (!sent) {
            LogError << "failed to send req" << VAR(req_id);
            return std::nullopt;
        }

        for (int64_t loop_count = 0;; ++loop_count) {
            // LogTrace << "enter loop" << VAR(req_id) << VAR(loop_count);

            // 嵌套等待时外层请求的回包可能落到内层循环里，由内层暂存
            if (auto pending = take_pending_response(req_id)) {
                if (!pending->is<ResponseT>()) {
                    LogError << "response type mismatch" << VAR(req_id) << VAR(*pending);
                    return std::nullopt;
                }
                return pending->as<ResponseT>();
            }

            auto msg_opt = recv();
            if (!msg_opt) {
                LogError << "failed to recv resp" << VAR(req_id) << VAR(loop_count);
                return std::nullopt;
            }
            const json::value& msg = *msg_opt;
            if (auto resp_id = msg.find<int64_t>(kRespIdKey)) {
                if (*resp_id == req_id && msg.is<ResponseT>()) {
                    return msg.as<ResponseT>();
                }
                stash_response(*resp_id, msg);
                continue;
            }
            // 对端不带编号（老版本）时按类型认
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
                dispatch_inserted_request(msg);
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
    bool dispatch_inserted_request(const json::value& j);
    bool handle_image_header(const json::value& j);
    bool handle_image_encoded_header(const json::value& j);

    void init_socket(const std::string& identifier, bool bind);
    void uninit_socket();
    void reset_socket(std::chrono::milliseconds linger = std::chrono::milliseconds(0));

    bool send(const json::value& j);
    bool send_no_wait(const json::value& j);
    std::optional<json::value> recv();

    bool alive();
    void set_timeout(const std::chrono::milliseconds& timeout);

private:
    void create_pair_socket();
    bool send_impl(const json::value& j);
    void handle_image(const ImageHeader& header);
    void handle_image_encoded(const ImageEncodedHeader& header);
    bool poll(zmq::pollitem_t& pollitem);

    static bool is_response(const json::value& j);
    void begin_request(int64_t req_id);
    void end_request(int64_t req_id);
    std::optional<json::value> take_pending_response(int64_t req_id);
    void stash_response(int64_t resp_id, const json::value& msg);

protected:
    // 返回实际绑定的端口号，如果传入 0 则自动选择可用端口
    uint16_t init_tcp_socket(uint16_t port, bool bind);

    // 纯数字 identifier 视为 TCP 端口号，范围限制为 1-65535
    static std::optional<uint16_t> parse_tcp_port(const std::string& identifier);

    // 检测 IPC 是否可能失败（Windows 下的路径问题等）
    static bool should_fallback_to_tcp();

protected:
    zmq::context_t zmq_ctx_;
    zmq::socket_t zmq_sock_;

    std::string ipc_addr_;
    std::filesystem::path ipc_path_;
    bool is_tcp_ = false;
    uint16_t tcp_port_ = 0;

    std::map<std::string /* uuid */, cv::Mat> recved_images_;
    std::map<std::string /* uuid */, ImageEncodedBuffer> recved_images_encoded_;

private:
    static constexpr const char* kReqIdKey = "_req_id";
    static constexpr const char* kRespIdKey = "_resp_id";

    inline static std::atomic<int64_t> s_req_id_ = 0;
    // 本线程正在处理的对端请求（嵌套时压栈），栈顶所属实例发出的回包带上其编号
    struct HandlingRequest
    {
        const Transceiver* owner = nullptr;
        std::optional<int64_t> req_id;
    };

    inline static thread_local std::vector<HandlingRequest> s_handling_requests_;

    // 只为仍在等待的请求暂存回包，两张表的大小都受限于同时在等的请求数
    std::mutex pending_mutex_;
    std::set<int64_t> active_req_ids_;
    std::map<int64_t, json::value> pending_responses_;

    bool is_bound_ = false;

    std::mutex socket_mutex_;
    zmq::pollitem_t zmq_pollitem_send_ { };
    zmq::pollitem_t zmq_pollitem_recv_ { };
    std::chrono::milliseconds timeout_ = std::chrono::milliseconds::max();
};

MAA_AGENT_NS_END
