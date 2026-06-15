#pragma once

#include <cstdint>
#include <string>

#include "Common/Conf.h"

struct ei;
struct ei_seat;
struct ei_device;
struct ei_event;

MAA_CTRL_UNIT_NS_BEGIN

class EiInput
{
public:
    enum class EventPhase
    {
        Began,
        Moved,
        Ended
    };

    explicit EiInput(std::string eis_socket_path);
    ~EiInput();

    EiInput(const EiInput&) = delete;
    EiInput& operator=(const EiInput&) = delete;

    bool init();

    bool connected() const { return connected_; }

    bool pointer(EventPhase phase, int x, int y, int contact);
    bool keyboard_key(EventPhase phase, int evdev_key);
    bool text_utf8(const std::string& text);
    bool relative_move(int dx, int dy);
    bool scroll(int dx, int dy);

    void shutdown();

private:
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
