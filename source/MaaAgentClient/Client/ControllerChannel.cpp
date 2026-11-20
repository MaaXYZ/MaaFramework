#include "ControllerChannel.h"

#include <algorithm>

#include "MaaFramework/MaaAPI.h"
#include "MaaUtils/Buffer/BufferTypes.hpp"
#include "MaaUtils/ScopeLeave.hpp"

MAA_AGENT_CLIENT_NS_BEGIN

namespace
{
struct ControllerProxy
{
    std::weak_ptr<ControllerChannel> channel;
    std::string name;

    static CustomControllerResponse
        call(void* arg, CustomControllerRequest req, MaaStringBuffer* text = nullptr, MaaImageBuffer* image = nullptr)
    {
        auto& self = *static_cast<ControllerProxy*>(arg);
        auto channel = self.channel.lock();
        if (!channel) {
            return {};
        }
        req.name = self.name;
        auto resp = channel->invoke(req, image);
        if (resp.success && text) {
            text->set(resp.text);
        }
        return resp;
    }
};

MaaCustomControllerCallbacks callbacks {
    .connect = [](void* arg) -> MaaBool { return ControllerProxy::call(arg, { .method = "connect" }).success; },
    .connected = [](void* arg) -> MaaBool { return ControllerProxy::call(arg, { .method = "connected" }).success; },
    .request_uuid = [](void* arg, MaaStringBuffer* out) -> MaaBool {
        return ControllerProxy::call(arg, { .method = "request_uuid" }, out).success;
    },
    .get_features = [](void* arg) -> MaaControllerFeature { return ControllerProxy::call(arg, { .method = "get_features" }).features; },
    .start_app = [](const char* intent, void* arg) -> MaaBool {
        return ControllerProxy::call(arg, { .method = "start_app", .text = intent }).success;
    },
    .stop_app = [](const char* intent, void* arg) -> MaaBool {
        return ControllerProxy::call(arg, { .method = "stop_app", .text = intent }).success;
    },
    .screencap = [](void* arg, MaaImageBuffer* out) -> MaaBool {
        return ControllerProxy::call(arg, { .method = "screencap" }, nullptr, out).success;
    },
    .click = [](int32_t x, int32_t y, void* arg) -> MaaBool {
        return ControllerProxy::call(arg, { .method = "click", .args = { x, y } }).success;
    },
    .swipe = [](int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t duration, void* arg) -> MaaBool {
        return ControllerProxy::call(arg, { .method = "swipe", .args = { x1, y1, x2, y2, duration } }).success;
    },
    .touch_down = [](int32_t contact, int32_t x, int32_t y, int32_t pressure, void* arg) -> MaaBool {
        return ControllerProxy::call(arg, { .method = "touch_down", .args = { contact, x, y, pressure } }).success;
    },
    .touch_move = [](int32_t contact, int32_t x, int32_t y, int32_t pressure, void* arg) -> MaaBool {
        return ControllerProxy::call(arg, { .method = "touch_move", .args = { contact, x, y, pressure } }).success;
    },
    .touch_up = [](int32_t contact, void* arg) -> MaaBool {
        return ControllerProxy::call(arg, { .method = "touch_up", .args = { contact } }).success;
    },
    .click_key = [](int32_t keycode, void* arg) -> MaaBool {
        return ControllerProxy::call(arg, { .method = "click_key", .args = { keycode } }).success;
    },
    .input_text = [](const char* text, void* arg) -> MaaBool {
        return ControllerProxy::call(arg, { .method = "input_text", .text = text }).success;
    },
    .key_down = [](int32_t keycode, void* arg) -> MaaBool {
        return ControllerProxy::call(arg, { .method = "key_down", .args = { keycode } }).success;
    },
    .key_up = [](int32_t keycode, void* arg) -> MaaBool {
        return ControllerProxy::call(arg, { .method = "key_up", .args = { keycode } }).success;
    },
    .scroll = [](int32_t dx, int32_t dy, void* arg) -> MaaBool {
        return ControllerProxy::call(arg, { .method = "scroll", .args = { dx, dy } }).success;
    },
    .relative_move = [](int32_t dx, int32_t dy, void* arg) -> MaaBool {
        return ControllerProxy::call(arg, { .method = "relative_move", .args = { dx, dy } }).success;
    },
    .shell = [](const char* cmd, int64_t timeout, void* arg, MaaStringBuffer* out) -> MaaBool {
        return ControllerProxy::call(arg, { .method = "shell", .text = cmd, .timeout = timeout }, out).success;
    },
    .inactive = [](void* arg) -> MaaBool { return ControllerProxy::call(arg, { .method = "inactive" }).success; },
    .get_info = [](void* arg, MaaStringBuffer* out) -> MaaBool {
        return ControllerProxy::call(arg, { .method = "get_info" }, out).success;
    },
};
} // namespace

std::shared_ptr<ControllerChannel> ControllerChannel::open(std::string_view endpoint, bool tcp, const std::vector<std::string>& names)
{
    auto channel = std::make_shared<ControllerChannel>();
    channel->watch_peer_disconnect(true);
    try {
        if (tcp) {
            auto port = parse_tcp_port(std::string(endpoint));
            if (!port) {
                return nullptr;
            }
            channel->init_tcp_socket(*port, false);
        }
        else {
            if (endpoint.empty() || endpoint.size() > 64 || endpoint.find_first_not_of("0123456789abcdefABCDEF-") != std::string_view::npos) {
                return nullptr;
            }
            channel->init_socket(std::string(endpoint), false);
        }
        channel->zmq_sock_.set(zmq::sockopt::linger, 0);
    }
    catch (const zmq::error_t& e) {
        LogError << "failed to open controller channel" << e.what();
        return nullptr;
    }
    channel->names_ = names;
    return channel;
}

MaaController* ControllerChannel::create(const std::string& name)
{
    if (std::ranges::find(names_, name) == names_.end()) {
        return nullptr;
    }
    auto* proxy = new ControllerProxy { weak_from_this(), name };
    return MaaCustomControllerCreateWithRelease(&callbacks, proxy, [](void* arg) { delete static_cast<ControllerProxy*>(arg); });
}

void ControllerChannel::close()
{
    closed_ = true;
    // Interrupt a controller RPC even when its configured receive timeout is infinite.
    zmq_ctx_.shutdown();
}

CustomControllerResponse ControllerChannel::invoke(const CustomControllerRequest& req, MaaImageBuffer* image)
{
    std::scoped_lock lock(call_mutex_);
    if (closed_) {
        return {};
    }

    // A timed-out screencap may arrive during the next RPC. Only that RPC's image is consumed.
    OnScopeLeave([&]() { recved_images_.clear(); });
    try {
        auto resp = send_and_recv<CustomControllerResponse>(req);
        if (!resp || !resp->success) {
            return {};
        }
        if (image) {
            auto mat = get_image_cache(resp->image);
            if (mat.empty()) {
                return {};
            }
            image->set(std::move(mat));
        }
        return std::move(*resp);
    }
    catch (const zmq::error_t& e) {
        if (e.num() != ETERM) {
            LogError << "controller channel error" << e.what();
        }
        return {};
    }
}

MAA_AGENT_CLIENT_NS_END
