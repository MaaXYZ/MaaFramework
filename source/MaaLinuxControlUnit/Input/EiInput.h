#pragma once

#include <cstdint>
#include <string>

#include "Base/UnitBase.h"
#include "Common/Conf.h"

struct ei;
struct ei_seat;
struct ei_device;
struct ei_event;

MAA_CTRL_UNIT_NS_BEGIN

// 通过 libei (EIS socket) 注入输入事件。
// EIS socket 由合成器提供 (如 gamescope 的 /run/user/<uid>/gamescope-<n>-ei),
// 绝对坐标空间与合成器的输出一致。
class EiInput : public RelativeMoveInput
{
public:
    explicit EiInput(std::string eis_socket_path);
    ~EiInput() override;

    EiInput(const EiInput&) = delete;
    EiInput& operator=(const EiInput&) = delete;

    bool connected() const { return connected_; }

    bool init() override;
    MaaControllerFeature get_features() const override;

    bool click(int x, int y) override;
    bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    bool touch_down(int contact, int x, int y, int pressure) override;
    bool touch_move(int contact, int x, int y, int pressure) override;
    bool touch_up(int contact) override;

    bool click_key(int key) override;
    bool input_text(const std::string& text) override;

    bool key_down(int key) override;
    bool key_up(int key) override;

    bool scroll(int dx, int dy) override;
    bool relative_move(int dx, int dy) override;

    void shutdown();

private:
    bool pointer(EventPhase phase, int x, int y, int contact);
    bool keyboard_key(EventPhase phase, int evdev_key);
    bool text_utf8(const std::string& text);

    int poll_and_dispatch(int timeout_ms);
    void handle_event(struct ei_event* event);

    template <typename F>
    bool send(F&& emit);

    struct ei* ei_ = nullptr;
    struct ei_device* device_ = nullptr;

    int ei_fd_ = -1;

    std::string eis_socket_path_;

    bool connected_ = false;

    uint64_t emul_seq_ = 0;
};

MAA_CTRL_UNIT_NS_END
