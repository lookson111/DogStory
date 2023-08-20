#pragma once
#include <boost/asio.hpp>
#include <chrono>

namespace ticker {
namespace net = boost::asio;
namespace sys = boost::system;
using namespace std::chrono;
class Ticker : public std::enable_shared_from_this<Ticker> {
public:
    using Strand = net::strand<net::io_context::executor_type>;
    using Handler = std::function<void(milliseconds delta)>;

    // Функция handler будет вызываться внутри strand с интервалом period
    Ticker(Strand strand, milliseconds period, Handler handler)
        : strand_{ strand }
        , period_{ period }
        , handler_{ std::move(handler) } {
    }

    void Start() {
        if (period_ == milliseconds(0))
            return;
        net::dispatch(strand_, [self = shared_from_this()] {
            self->last_tick_ = Clock::now();
            self->ScheduleTick();
            });
    }

private:
    using Clock = steady_clock;
    Strand strand_;
    milliseconds period_;
    net::steady_timer timer_{strand_};
    Handler handler_;
    steady_clock::time_point last_tick_;
    
    void ScheduleTick() {
        assert(strand_.running_in_this_thread());
        timer_.expires_after(period_);
        timer_.async_wait([self = shared_from_this()](sys::error_code ec) {
            self->OnTick(ec);
            });
    }

    void OnTick(sys::error_code ec) {
        using namespace std::chrono;
        if (!ec) {
            auto this_tick = Clock::now();
            auto delta = duration_cast<milliseconds>(this_tick - last_tick_);
            last_tick_ = this_tick;
            try {
                handler_(delta);
            }
            catch (...) {
            }
            ScheduleTick();
        }
    }
};
}
