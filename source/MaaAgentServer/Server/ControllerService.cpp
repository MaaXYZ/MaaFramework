#include "ControllerService.h"

#include "MaaUtils/Buffer/ImageBuffer.hpp"
#include "MaaUtils/Buffer/StringBuffer.hpp"
#include "MaaUtils/Uuid.h"

MAA_AGENT_SERVER_NS_BEGIN

ControllerService::ControllerService(std::map<std::string, ControllerRegistration> registrations)
    : registrations_(std::move(registrations))
{
}

ControllerService::~ControllerService()
{
    if (thread_.joinable()) {
        zmq_ctx_.shutdown();
        thread_.join();
    }
}

std::string ControllerService::start(bool tcp)
{
    std::string endpoint;
    if (tcp) {
        endpoint = std::to_string(init_tcp_socket(0, true));
    }
    else {
        endpoint = make_uuid();
        init_socket(endpoint, true);
    }
    thread_ = std::thread(&ControllerService::run, this);
    return endpoint;
}

void ControllerService::run()
{
    is_agent_server_thread = true;
    try {
        while (auto msg = recv()) {
            dispatch_inserted_request(*msg);
        }
    }
    catch (const zmq::error_t& e) {
        if (e.num() != ETERM) {
            LogError << "controller channel error" << e.what();
        }
    }
}

bool ControllerService::handle_inserted_request(const json::value& j)
{
    CustomControllerResponse resp;
    if (j.is<CustomControllerRequest>()) {
        const auto req = j.as<CustomControllerRequest>();
        auto it = registrations_.find(req.name);
        if (it != registrations_.end()) {
            resp = invoke(it->second, req);
        }
    }
    return send(resp);
}

CustomControllerResponse ControllerService::invoke(const ControllerRegistration& registration, const CustomControllerRequest& req)
{
    const auto& cb = registration.callbacks;
    auto* arg = registration.arg;
    const auto& args = req.args;
    CustomControllerResponse resp;
    if (req.method == "screencap" && cb.screencap) {
        ImageBuffer image;
        if (cb.screencap(arg, &image) && !image.empty()) {
            resp.image = send_image(image.get());
            resp.success = !resp.image.empty();
        }
    }
    else if (req.method == "get_features") {
        resp.features = cb.get_features ? cb.get_features(arg) : MaaControllerFeature_None;
        resp.success = true;
    }
    else if (req.method == "connected") {
        resp.success = !cb.connected || cb.connected(arg);
    }
    else if (req.method == "inactive") {
        resp.success = !cb.inactive || cb.inactive(arg);
    }
    else if (req.method == "request_uuid" || req.method == "get_info" || req.method == "shell") {
        StringBuffer buffer;
        if (req.method == "request_uuid" && cb.request_uuid) {
            resp.success = cb.request_uuid(arg, &buffer);
        }
        else if (req.method == "get_info" && cb.get_info) {
            resp.success = cb.get_info(arg, &buffer);
        }
        else if (req.method == "shell" && cb.shell) {
            resp.success = cb.shell(req.text.c_str(), req.timeout, arg, &buffer);
        }
        if (resp.success) {
            resp.text = buffer.get();
        }
    }
    else if (req.method == "connect" && cb.connect) {
        resp.success = cb.connect(arg);
    }
    else if (req.method == "start_app" && cb.start_app) {
        resp.success = cb.start_app(req.text.c_str(), arg);
    }
    else if (req.method == "stop_app" && cb.stop_app) {
        resp.success = cb.stop_app(req.text.c_str(), arg);
    }
    else if (req.method == "click" && cb.click && args.size() == 2) {
        resp.success = cb.click(args[0], args[1], arg);
    }
    else if (req.method == "swipe" && cb.swipe && args.size() == 5) {
        resp.success = cb.swipe(args[0], args[1], args[2], args[3], args[4], arg);
    }
    else if (req.method == "touch_down" && cb.touch_down && args.size() == 4) {
        resp.success = cb.touch_down(args[0], args[1], args[2], args[3], arg);
    }
    else if (req.method == "touch_move" && cb.touch_move && args.size() == 4) {
        resp.success = cb.touch_move(args[0], args[1], args[2], args[3], arg);
    }
    else if (req.method == "touch_up" && cb.touch_up && args.size() == 1) {
        resp.success = cb.touch_up(args[0], arg);
    }
    else if (req.method == "click_key" && cb.click_key && args.size() == 1) {
        resp.success = cb.click_key(args[0], arg);
    }
    else if (req.method == "input_text" && cb.input_text) {
        resp.success = cb.input_text(req.text.c_str(), arg);
    }
    else if (req.method == "key_down" && cb.key_down && args.size() == 1) {
        resp.success = cb.key_down(args[0], arg);
    }
    else if (req.method == "key_up" && cb.key_up && args.size() == 1) {
        resp.success = cb.key_up(args[0], arg);
    }
    else if (req.method == "scroll" && cb.scroll && args.size() == 2) {
        resp.success = cb.scroll(args[0], args[1], arg);
    }
    else if (req.method == "relative_move" && cb.relative_move && args.size() == 2) {
        resp.success = cb.relative_move(args[0], args[1], arg);
    }
    return resp;
}

MAA_AGENT_SERVER_NS_END
