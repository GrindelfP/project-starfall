#include "frame_timer.h"
#include "input_state.h"
#include <chrono>
#include <cstdlib>
#include <iostream>

void check(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}
int main() {
    InputState input;
    input.keyDown('W'); input.keyDown('W');
    check(input.held('W') && input.pressed('W') && !input.released('W'), "initial press");
    input.beginFrame(); input.keyDown('W');
    check(input.held('W') && !input.pressed('W'), "repeat must not be a press");
    input.keyUp('W'); input.keyDown('W');
    check(input.held('W') && input.pressed('W') && input.released('W'), "both transitions within frame");
    input.addMouse(4, -3); input.addMouse(-1, 5);
    check(input.mouseX() == 3 && input.mouseY() == 2, "mouse accumulation");
    input.clear();
    check(!input.held('W') && input.released('W') && input.mouseX() == 0, "focus loss clears input");
    input.beginFrame();
    check(!input.pressed('W') && !input.released('W') && input.mouseY() == 0, "frame transitions reset");
    input.keyDown(256); check(!input.held(256), "invalid key ignored");
    FrameTimer timer;
    const FrameTimer::Clock::time_point start{};
    using namespace std::chrono_literals;
    check(timer.tick(start) == 0.0, "first tick zero");
    check(timer.tick(start + 16ms) > 0.015 && timer.tick(start + 32ms) < 0.017, "normal delta");
    check(timer.tick(start + 10s) == FrameTimer::maxDeltaSeconds, "long frame clamped");
    timer.reset();
    check(timer.tick(start + 60s) == 0.0, "resume delta reset");
    check(timer.tick(start + 60s + 16ms) > 0.015, "clock resumes");
    std::cout << "Input and timing tests passed\n";
}
