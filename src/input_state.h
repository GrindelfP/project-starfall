#pragma once
#include <array>
#include <cstddef>

// One state per virtual key. Transitions last until the next beginFrame().
class InputState {
public:
    static constexpr std::size_t keyCount = 256;
    void beginFrame() { pressed_.fill(false); released_.fill(false); mouseX_ = mouseY_ = 0; }
    void keyDown(std::size_t key) {
        if (key < keyCount && !held_[key]) { held_[key] = true; pressed_[key] = true; }
    }
    void keyUp(std::size_t key) {
        if (key < keyCount && held_[key]) { held_[key] = false; released_[key] = true; }
    }
    void clear() {
        for (std::size_t i = 0; i < keyCount; ++i) if (held_[i]) keyUp(i);
        mouseX_ = mouseY_ = 0;
    }
    void addMouse(long x, long y) { mouseX_ += x; mouseY_ += y; }
    bool held(std::size_t key) const { return key < keyCount && held_[key]; }
    bool pressed(std::size_t key) const { return key < keyCount && pressed_[key]; }
    bool released(std::size_t key) const { return key < keyCount && released_[key]; }
    long mouseX() const { return mouseX_; }
    long mouseY() const { return mouseY_; }
private:
    std::array<bool, keyCount> held_{}, pressed_{}, released_{};
    long mouseX_ = 0, mouseY_ = 0;
};
