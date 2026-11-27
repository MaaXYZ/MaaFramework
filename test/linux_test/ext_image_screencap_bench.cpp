#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <format>
#include <iostream>
#include <limits>
#include <poll.h>
#include <string>
#include <vector>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "ext-image-capture-source-v1.h"
#include "ext-image-copy-capture-v1.h"
#include "wayland-client-protocol.h"
#include "wayland-client.h"

// Manual benchmark for the two ext-image-copy-capture-v1 session strategies.
// Requires a compositor that implements the protocol (e.g. wlroots/sway >= 0.19).
//
//   A) fresh session per capture: what ExtImageScreencap does. Every frame is
//      the session's first frame, which the compositor must copy immediately.
//   B) persistent session: reused for comparison. Per the protocol, captures
//      after the first may block until the source content changes, so this
//      strategy degrades whenever the screen is idle.
//
// Strategy C re-checks B on a static screen with a bounded wait, demonstrating
// the hang risk that motivates the per-capture session in the control unit.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

namespace chrono = std::chrono;

namespace
{

struct State
{
    // globals
    wl_display* display = nullptr;
    wl_registry* registry = nullptr;
    wl_output* output = nullptr;
    wl_shm* shm = nullptr;
    ext_output_image_capture_source_manager_v1* source_manager = nullptr;
    ext_image_copy_capture_manager_v1* capture_manager = nullptr;

    // session constraints
    bool constraints_done = false;
    bool session_stopped = false;
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint32_t> shm_formats;

    // frame
    bool capture_done = false;
    bool capture_ok = false;

    // shm buffer
    int memfd = -1;
    void* mem = nullptr;
    size_t mem_size = 0;
    wl_shm_pool* pool = nullptr;
    wl_buffer* buffer = nullptr;
    uint32_t buffer_format = 0;
    uint32_t buffer_width = 0;
    uint32_t buffer_height = 0;
};

int bytes_per_pixel(uint32_t format)
{
    switch (format) {
    case WL_SHM_FORMAT_XBGR8888:
    case WL_SHM_FORMAT_ABGR8888:
    case WL_SHM_FORMAT_ARGB8888:
    case WL_SHM_FORMAT_XRGB8888:
        return 4;
    case WL_SHM_FORMAT_RGB888:
    case WL_SHM_FORMAT_BGR888:
        return 3;
    default:
        return -1;
    }
}

void registry_global(void*, wl_registry* registry, uint32_t name, const char* interface, uint32_t version)
{
    auto state = static_cast<State*>(wl_registry_get_user_data(registry));
    auto bind = [&](wl_proxy** target, const wl_interface* ifc) {
        *target = static_cast<wl_proxy*>(wl_registry_bind(registry, name, ifc, version));
    };

    if (std::strcmp(interface, "wl_output") == 0) {
        bind(reinterpret_cast<wl_proxy**>(&state->output), &wl_output_interface);
    }
    else if (std::strcmp(interface, "wl_shm") == 0) {
        bind(reinterpret_cast<wl_proxy**>(&state->shm), &wl_shm_interface);
    }
    else if (std::strcmp(interface, "ext_output_image_capture_source_manager_v1") == 0) {
        bind(reinterpret_cast<wl_proxy**>(&state->source_manager), &ext_output_image_capture_source_manager_v1_interface);
    }
    else if (std::strcmp(interface, "ext_image_copy_capture_manager_v1") == 0) {
        bind(reinterpret_cast<wl_proxy**>(&state->capture_manager), &ext_image_copy_capture_manager_v1_interface);
    }
}

constexpr wl_registry_listener registry_listener = { registry_global, nullptr };

void session_buffer_size(void* data, ext_image_copy_capture_session_v1*, uint32_t width, uint32_t height)
{
    auto state = static_cast<State*>(data);
    state->width = width;
    state->height = height;
}

void session_shm_format(void* data, ext_image_copy_capture_session_v1*, uint32_t format)
{
    static_cast<State*>(data)->shm_formats.push_back(format);
}

void session_dmabuf_device(void*, ext_image_copy_capture_session_v1*, wl_array*)
{
    // shm buffers only
}

void session_dmabuf_format(void*, ext_image_copy_capture_session_v1*, uint32_t, wl_array*)
{
    // shm buffers only
}

void session_done(void* data, ext_image_copy_capture_session_v1*)
{
    static_cast<State*>(data)->constraints_done = true;
}

void session_stopped(void* data, ext_image_copy_capture_session_v1*)
{
    static_cast<State*>(data)->session_stopped = true;
}

constexpr ext_image_copy_capture_session_v1_listener session_listener = { session_buffer_size,
                                                                          session_shm_format,
                                                                          session_dmabuf_device,
                                                                          session_dmabuf_format,
                                                                          session_done,
                                                                          session_stopped };

void frame_transform(void*, ext_image_copy_capture_frame_v1*, uint32_t)
{
    // nop
}

void frame_damage(void*, ext_image_copy_capture_frame_v1*, int32_t, int32_t, int32_t, int32_t)
{
    // nop
}

void frame_presentation_time(void*, ext_image_copy_capture_frame_v1*, uint32_t, uint32_t, uint32_t)
{
    // nop
}

void frame_ready(void* data, ext_image_copy_capture_frame_v1*)
{
    auto state = static_cast<State*>(data);
    state->capture_done = true;
    state->capture_ok = true;
}

void frame_failed(void* data, ext_image_copy_capture_frame_v1*, uint32_t reason)
{
    auto state = static_cast<State*>(data);
    state->capture_done = true;
    state->capture_ok = false;
    std::cerr << std::format("  frame failed, reason = {}", reason) << std::endl;
}

constexpr ext_image_copy_capture_frame_v1_listener frame_listener = { frame_transform,
                                                                      frame_damage,
                                                                      frame_presentation_time,
                                                                      frame_ready,
                                                                      frame_failed };

// Canonical non-blocking Wayland dispatch loop. Returns 0 once the predicate
// fired, 1 on timeout, -1 on a protocol or IO error.
template <typename Pred>
int wait_until(State* state, Pred pred, int timeout_ms)
{
    while (!pred()) {
        if (wl_display_prepare_read(state->display) != 0) {
            if (wl_display_dispatch_pending(state->display) < 0) {
                return -1;
            }
            continue;
        }
        if (wl_display_flush(state->display) < 0 && errno != EAGAIN) {
            wl_display_cancel_read(state->display);
            return -1;
        }
        pollfd p = { wl_display_get_fd(state->display), POLLIN, 0 };
        int result = poll(&p, 1, timeout_ms);
        if (result == 0) {
            wl_display_cancel_read(state->display);
            return 1;
        }
        if (result < 0) {
            wl_display_cancel_read(state->display);
            return -1;
        }
        wl_display_read_events(state->display);
        if (wl_display_dispatch_pending(state->display) < 0) {
            return -1;
        }
    }
    return 0;
}

bool make_buffer(State* state)
{
    uint32_t picked = 0;
    int bpp = -1;
    for (uint32_t format : state->shm_formats) {
        if (bytes_per_pixel(format) > 0) {
            picked = format;
            bpp = bytes_per_pixel(format);
            break;
        }
    }
    if (bpp < 0) {
        std::cerr << std::format("no supported shm format ({} offered)", state->shm_formats.size()) << std::endl;
        return false;
    }
    if (state->buffer && state->buffer_format == picked && state->buffer_width == state->width
        && state->buffer_height == state->height) {
        return true;
    }

    if (state->buffer) {
        wl_buffer_destroy(state->buffer);
        state->buffer = nullptr;
    }
    if (state->pool) {
        wl_shm_pool_destroy(state->pool);
        state->pool = nullptr;
    }
    if (state->mem) {
        munmap(state->mem, state->mem_size);
        state->mem = nullptr;
    }
    if (state->memfd >= 0) {
        close(state->memfd);
    }

    uint32_t stride = state->width * bpp;
    state->mem_size = static_cast<size_t>(stride) * state->height;
    state->memfd = memfd_create("ext_image_screencap_bench", 0);
    if (ftruncate(state->memfd, static_cast<off_t>(state->mem_size)) < 0) {
        return false;
    }
    state->mem = mmap(nullptr, state->mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, state->memfd, 0);
    if (state->mem == MAP_FAILED) {
        return false;
    }
    state->pool = wl_shm_create_pool(state->shm, state->memfd, static_cast<int32_t>(state->mem_size));
    state->buffer = wl_shm_pool_create_buffer(state->pool, 0, static_cast<int32_t>(state->width),
                                               static_cast<int32_t>(state->height), static_cast<int32_t>(stride),
                                               picked);
    state->buffer_format = picked;
    state->buffer_width = state->width;
    state->buffer_height = state->height;
    wl_display_roundtrip(state->display);
    return true;
}

void destroy_frame(ext_image_copy_capture_frame_v1* frame)
{
    ext_image_copy_capture_frame_v1_destroy(frame);
}

// Strategy A: a fresh source/session per capture, the flow ExtImageScreencap runs.
bool capture_fresh_session(State* state, int timeout_ms)
{
    state->constraints_done = false;
    state->session_stopped = false;
    state->width = state->height = 0;
    state->shm_formats.clear();

    auto* source = ext_output_image_capture_source_manager_v1_create_source(state->source_manager, state->output);
    auto* session = ext_image_copy_capture_manager_v1_create_session(state->capture_manager, source, 0);
    ext_image_copy_capture_session_v1_add_listener(session, &session_listener, state);

    if (wait_until(state, [&] { return state->constraints_done || state->session_stopped; }, timeout_ms) != 0) {
        std::cerr << "  constraint wait failed" << std::endl;
        ext_image_copy_capture_session_v1_destroy(session);
        ext_image_capture_source_v1_destroy(source);
        return false;
    }
    if (state->session_stopped) {
        std::cerr << "  session stopped before constraints" << std::endl;
        ext_image_copy_capture_session_v1_destroy(session);
        ext_image_capture_source_v1_destroy(source);
        return false;
    }
    if (!make_buffer(state)) {
        ext_image_copy_capture_session_v1_destroy(session);
        ext_image_capture_source_v1_destroy(source);
        return false;
    }

    state->capture_done = false;
    state->capture_ok = false;
    auto* frame = ext_image_copy_capture_session_v1_create_frame(session);
    ext_image_copy_capture_frame_v1_add_listener(frame, &frame_listener, state);
    ext_image_copy_capture_frame_v1_attach_buffer(frame, state->buffer);
    ext_image_copy_capture_frame_v1_damage_buffer(frame, 0, 0, static_cast<int32_t>(state->width),
                                                   static_cast<int32_t>(state->height));
    ext_image_copy_capture_frame_v1_capture(frame);

    bool ok = wait_until(state, [&] { return state->capture_done; }, timeout_ms) == 0 && state->capture_ok;
    destroy_frame(frame);
    ext_image_copy_capture_session_v1_destroy(session);
    ext_image_capture_source_v1_destroy(source);
    return ok;
}

// Strategy B: one frame on a session that stays alive across captures.
bool capture_persistent_session(State* state, ext_image_copy_capture_session_v1* session, int timeout_ms)
{
    state->capture_done = false;
    state->capture_ok = false;
    auto* frame = ext_image_copy_capture_session_v1_create_frame(session);
    ext_image_copy_capture_frame_v1_add_listener(frame, &frame_listener, state);
    ext_image_copy_capture_frame_v1_attach_buffer(frame, state->buffer);
    ext_image_copy_capture_frame_v1_damage_buffer(frame, 0, 0, static_cast<int32_t>(state->buffer_width),
                                                   static_cast<int32_t>(state->buffer_height));
    ext_image_copy_capture_frame_v1_capture(frame);
    bool ok = wait_until(state, [&] { return state->capture_done; }, timeout_ms) == 0 && state->capture_ok;
    destroy_frame(frame);
    return ok;
}

struct Stats
{
    double avg_ms = 0.0;
    double min_ms = 0.0;
    double max_ms = 0.0;
    int failures = 0;
};

template <typename Fn>
Stats run(const char* label, int iterations, Fn fn, State* state, int timeout_ms)
{
    Stats stats;
    stats.min_ms = std::numeric_limits<double>::max();
    double total = 0.0;
    for (int i = 0; i < iterations; ++i) {
        auto start = chrono::steady_clock::now();
        bool ok = fn(state, timeout_ms);
        double ms = chrono::duration<double, std::milli>(chrono::steady_clock::now() - start).count();
        total += ms;
        if (!ok) {
            ++stats.failures;
        }
        stats.min_ms = std::min(stats.min_ms, ms);
        stats.max_ms = std::max(stats.max_ms, ms);
    }
    stats.avg_ms = total / iterations;
    std::cout << std::format("{:<28} avg {:8.3f} ms  min {:8.3f}  max {:8.3f}  failures {}", label, stats.avg_ms,
                             stats.min_ms, stats.max_ms, stats.failures)
              << std::endl;
    return stats;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc > 3) {
        std::cerr << std::format("usage: {} [wayland-socket] [iterations]", argv[0]) << std::endl;
        return 1;
    }
    std::string socket = argc > 1 ? argv[1] : std::getenv("WAYLAND_DISPLAY") ? std::getenv("WAYLAND_DISPLAY") : "";
    if (socket.empty()) {
        std::cerr << "no wayland socket given and WAYLAND_DISPLAY is unset" << std::endl;
        return 1;
    }
    int iterations = argc > 2 ? std::atoi(argv[2]) : 60;

    State state;
    state.display = wl_display_connect(socket.c_str());
    if (!state.display) {
        std::cerr << std::format("failed to connect wayland socket {}", socket) << std::endl;
        return 1;
    }
    state.registry = wl_display_get_registry(state.display);
    wl_registry_add_listener(state.registry, &registry_listener, nullptr);
    wl_registry_set_user_data(state.registry, &state);
    if (wl_display_roundtrip(state.display) < 0 || !state.output || !state.shm || !state.source_manager
        || !state.capture_manager) {
        std::cerr << std::format("missing globals, compositor must support ext_image_capture_source_v1 and "
                                 "ext_image_copy_capture_v1 (output={} shm={} source_manager={} capture_manager={})",
                                 static_cast<void*>(state.output), static_cast<void*>(state.shm),
                                 static_cast<void*>(state.source_manager), static_cast<void*>(state.capture_manager))
                  << std::endl;
        return 1;
    }
    std::cout << std::format("connected to {}, {} iterations per strategy", socket, iterations) << std::endl;

    run("A: fresh session/capture", iterations, capture_fresh_session, &state, 5000);
    std::cout << std::format("  screen {}x{}, {} bytes per frame", state.buffer_width, state.buffer_height,
                             state.mem_size)
              << std::endl;

    // Strategy B: persistent session
    state.constraints_done = false;
    state.session_stopped = false;
    auto* source = ext_output_image_capture_source_manager_v1_create_source(state.source_manager, state.output);
    auto* session = ext_image_copy_capture_manager_v1_create_session(state.capture_manager, source, 0);
    ext_image_copy_capture_session_v1_add_listener(session, &session_listener, &state);
    if (wait_until(&state, [&] { return state.constraints_done || state.session_stopped; }, 5000) != 0
        || state.session_stopped || !make_buffer(&state)) {
        std::cerr << "persistent session setup failed" << std::endl;
        return 1;
    }

    run("B: persistent session", iterations,
        [&](State* s, int timeout_ms) { return capture_persistent_session(s, session, timeout_ms); }, &state, 5000);

    // Strategy C: let the screen settle, then check whether B still returns.
    std::cout << "C: static screen, waiting 3s for the screen to settle..." << std::endl;
    sleep(3);
    auto start = chrono::steady_clock::now();
    bool ok = capture_persistent_session(&state, session, 5000);
    double ms = chrono::duration<double, std::milli>(chrono::steady_clock::now() - start).count();
    if (ok) {
        std::cout << std::format("   persistent session capture returned in {:.3f} ms", ms) << std::endl;
    }
    else {
        std::cout << "   persistent session capture TIMED OUT (>5000 ms) on a static screen" << std::endl;
    }

    ext_image_copy_capture_session_v1_destroy(session);
    ext_image_capture_source_v1_destroy(source);
    wl_display_disconnect(state.display);
    return 0;
}
