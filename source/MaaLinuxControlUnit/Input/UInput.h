#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <linux/input.h>
#include <mutex>
#include <string>
#include <thread>

#include "Base/UnitBase.h"
#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class UInput : public RelativeMoveInput
{
public:
    UInput();
    ~UInput() override;

    UInput(const UInput&) = delete;
    UInput& operator=(const UInput&) = delete;

    bool open(const std::filesystem::path& device_node, int screen_width, int screen_height);
    void close();
    bool connected() const;
    MaaControllerFeature get_features() const override;
    bool click_key(int key) override;

    bool click(int x, int y) override;
    bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    bool touch_down(int contact, int x, int y, int pressure) override;
    bool touch_move(int contact, int x, int y, int pressure) override;
    bool touch_up(int contact) override;
    bool input_text(const std::string& str) override;

    bool scroll(int dx, int dy) override;
    bool relative_move(int dx, int dy) override;

    bool key_down(int key_code) override;
    bool key_up(int key_code) override;

    std::pair<int, int> screen_size() const;

private:
    bool create_device();
    bool destroy_device();
    bool emit_abs(int code, int value);
    bool emit_key(int code, int value);
    bool emit_syn();

    // Send absolute position + btn_code=1 + SYN
    bool send_pointer_down(int x, int y, int btn_code = BTN_LEFT);
    // Send absolute position + SYN (button state unchanged)
    bool send_pointer_move(int x, int y);
    // Send btn_code=0 + SYN
    bool send_pointer_up(int btn_code = BTN_LEFT);

    static uint64_t now_ms();

    int fd_ = -1;
    std::atomic<bool> connected_ { false };

    int screen_width_ = 0;
    int screen_height_ = 0;

    std::filesystem::path device_node_;

    // Track whether the pointer button is currently pressed
    bool pointer_down_ = false;

    mutable std::mutex mutex_;
};

MAA_CTRL_UNIT_NS_END
