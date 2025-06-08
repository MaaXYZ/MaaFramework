/*
    Copyright (c) 2016-2017 ZeroMQ community
    Copyright (c) 2009-2011 250bpm s.r.o.
    Copyright (c) 2011 Botond Ballo
    Copyright (c) 2007-2009 iMatix Corporation

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal in the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.
*/
#pragma once

#include <zmq.h>
#include <zmq.hpp>

namespace zmq
{

class timers
{
public:
    using id_t = int;
    using fn_t = zmq_timer_fn;
    using timeout_result_t = std::optional<std::chrono::milliseconds>;

    timers()
        : _timers(zmq_timers_new())
    {
        if (_timers == nullptr) {
            throw zmq::error_t();
        }
    }

    timers(const timers& other) = delete;
    timers& operator=(const timers& other) = delete;

    ~timers()
    {
        int rc = zmq_timers_destroy(&_timers);
        ZMQ_ASSERT(rc == 0);
    }

    id_t add(std::chrono::milliseconds interval, zmq_timer_fn handler, void* arg)
    {
        id_t timer_id = zmq_timers_add(_timers, interval.count(), handler, arg);
        if (timer_id == -1) {
            throw zmq::error_t();
        }
        return timer_id;
    }

    void cancel(id_t timer_id)
    {
        int rc = zmq_timers_cancel(_timers, timer_id);
        if (rc == -1) {
            throw zmq::error_t();
        }
    }

    void set_interval(id_t timer_id, std::chrono::milliseconds interval)
    {
        int rc = zmq_timers_set_interval(_timers, timer_id, interval.count());
        if (rc == -1) {
            throw zmq::error_t();
        }
    }

    void reset(id_t timer_id)
    {
        int rc = zmq_timers_reset(_timers, timer_id);
        if (rc == -1) {
            throw zmq::error_t();
        }
    }

    timeout_result_t timeout() const
    {
        int timeout = zmq_timers_timeout(_timers);
        if (timeout == -1) {
            return timeout_result_t {};
        }
        return std::chrono::milliseconds { timeout };
    }

    void execute()
    {
        int rc = zmq_timers_execute(_timers);
        if (rc == -1) {
            throw zmq::error_t();
        }
    }

private:
    void* _timers;
};

} // namespace zmq
