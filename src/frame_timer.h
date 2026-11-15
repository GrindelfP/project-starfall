#pragma once
#include <algorithm>
#include <chrono>

// Variable-step updates: cap a frame at 100 ms and reset across pauses.
class FrameTimer {
public:
    using Clock = std::chrono::steady_clock;
    static constexpr double maxDeltaSeconds = 0.1;

    double tick(Clock::time_point now) {
        if (!running_) { last_ = now; running_ = true; return 0.0; }
        const double elapsed = std::chrono::duration<double>(now - last_).count();
        last_ = now;
        return std::clamp(elapsed, 0.0, maxDeltaSeconds);
    }
    void reset() { running_ = false; }
private:
    Clock::time_point last_{};
    bool running_ = false;
};
