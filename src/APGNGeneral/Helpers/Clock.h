// Clock.h
#pragma once

#include <chrono>

namespace ge::helpers {

// Simple singleton clock that tracks delta time and absolute time.
class Clock {
public:
    using steady_clock = std::chrono::steady_clock;
    static Clock& instance() {
        static Clock s;
        return s;
    }

    // Call once per frame/update
    void tick() {
        const auto now = steady_clock::now();
        std::chrono::duration<double> d = now - last_;
        delta_ = d.count();
        last_ = now;
        time_ = std::chrono::duration<double>(now - start_).count();
        ++frame_;
    }

    double delta() const noexcept { return delta_; }     // seconds since last tick
    double time() const noexcept { return time_; }       // seconds since clock start
    uint64_t frameCount() const noexcept { return frame_; }

private:
    Clock()
        : start_(steady_clock::now()), last_(start_), delta_(0.0), time_(0.0), frame_(0) {}
    steady_clock::time_point start_;
    steady_clock::time_point last_;
    double delta_;
    double time_;
    uint64_t frame_;
};

} // namespace ge::helpers