// cpp
#pragma once

#include <chrono>
#include <vector>
#include <mutex>
#include <cstdint>

namespace ge::helpers {

class Stopwatch {
public:
    using clock = std::chrono::steady_clock;
    using duration_type = clock::duration;

    Stopwatch() = default;

    // Start if not already running.
    void start() {
        std::lock_guard<std::mutex> guard(mutex_);
        if (!running_) {
            start_time_ = clock::now();
            running_ = true;
        }
    }

    // Stop and accumulate elapsed time.
    void stop() {
        std::lock_guard<std::mutex> guard(mutex_);
        if (running_) {
            elapsed_ += clock::now() - start_time_;
            running_ = false;
        }
    }

    // Reset to zero and clear laps.
    void reset() {
        std::lock_guard<std::mutex> guard(mutex_);
        running_ = false;
        elapsed_ = duration_type::zero();
        laps_.clear();
    }

    // Reset and start immediately.
    void restart() {
        std::lock_guard<std::mutex> guard(mutex_);
        elapsed_ = duration_type::zero();
        laps_.clear();
        start_time_ = clock::now();
        running_ = true;
    }

    // Record a lap and return lap time in seconds.
    double lap() {
        std::lock_guard<std::mutex> guard(mutex_);
        duration_type now_total = running_ ? (elapsed_ + (clock::now() - start_time_)) : elapsed_;
        laps_.push_back(now_total);
        return std::chrono::duration<double>(now_total).count();
    }

    // Return total elapsed time as chrono::duration<double>.
    std::chrono::duration<double> elapsed() const {
        std::lock_guard<std::mutex> guard(mutex_);
        duration_type total = running_ ? (elapsed_ + (clock::now() - start_time_)) : elapsed_;
        return std::chrono::duration<double>(total);
    }

    // Convenience getters.
    double elapsedSeconds() const { return elapsed().count(); }
    std::uint64_t elapsedMilliseconds() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(elapsed()).count();
    }
    std::uint64_t elapsedNanoseconds() const {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed()).count();
    }

    // Return lap times as seconds.
    std::vector<double> lapsSeconds() const {
        std::lock_guard<std::mutex> guard(mutex_);
        std::vector<double> out;
        out.reserve(laps_.size());
        for (auto const &d : laps_) out.emplace_back(std::chrono::duration<double>(d).count());
        return out;
    }

    bool isRunning() const {
        std::lock_guard<std::mutex> guard(mutex_);
        return running_;
    }

private:
    mutable std::mutex mutex_;
    duration_type elapsed_{duration_type::zero()};
    clock::time_point start_time_{};
    bool running_{false};
    std::vector<duration_type> laps_;
};

} // namespace ge::helpers