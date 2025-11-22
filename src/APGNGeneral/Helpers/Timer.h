// Timer.h
#pragma once

#include "Clock.h"

namespace ge::helpers {

// Simple one-shot timer (non-blocking)
class Timer {
public:
    explicit Timer(double durationSeconds = 0.0)
        : duration_(durationSeconds), running_(false) {}

    void start(double durationSeconds) {
        duration_ = durationSeconds;
        startTime_ = Clock::instance().time();
        running_ = true;
    }

    void start() { startTime_ = Clock::instance().time(); running_ = true; }

    void stop() { running_ = false; }

    bool expired() const {
        if (!running_) return true;
        return (Clock::instance().time() - startTime_) >= duration_;
    }

    double remaining() const {
        if (!running_) return 0.0;
        double rem = duration_ - (Clock::instance().time() - startTime_);
        return rem > 0.0 ? rem : 0.0;
    }

private:
    double duration_;
    double startTime_{0.0};
    bool running_;
};

} // namespace ge::helpers