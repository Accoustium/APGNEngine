// FPSCounter.h
#pragma once

#include "Clock.h"

namespace ge::helpers {

// Simple FPS counter (update per frame with delta)
class FPSCounter {
public:
    void update() {
        ++frames_;
        accumulated_ += Clock::instance().delta();
        if (accumulated_ >= sampleWindow_) {
            fps_ = static_cast<double>(frames_) / accumulated_;
            frames_ = 0;
            accumulated_ = 0.0;
        }
    }
    double fps() const noexcept { return fps_; }
    void setSampleWindow(double seconds) noexcept { sampleWindow_ = seconds; }

private:
    double sampleWindow_{1.0};
    int frames_{0};
    double accumulated_{0.0};
    double fps_{0.0};
};

} // namespace ge::helpers