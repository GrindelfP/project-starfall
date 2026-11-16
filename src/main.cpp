#include "render_backend.h"
#include "camera.h"
#include "ship.h"
#include "asteroid_field.h"
#include "hud.h"
#include <memory>
#include "frame_timer.h"
#include "input_state.h"
#include <windows.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <optional>
#include <dxgi1_4.h>

namespace {
struct WindowState {
    bool minimized = false;
    bool sizeChanged = true;
    std::ostream* log = nullptr;
    bool focused = false;
    bool captured = false;
    Hud* hud = nullptr;
    InputState input;
    FrameTimer timer;
};
LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<WindowState*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        state = static_cast<WindowState*>(create->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    }
    if (state) {
        if (message == WM_SIZE) {
            state->minimized = wParam == SIZE_MINIMIZED;
            state->sizeChanged = true;
            if (state->hud) state->hud->updateLayout();
            if (state->minimized) {
                state->timer.reset();
                state->input.clear();
                if (state->captured) ReleaseCapture();
            }
            *state->log << (state->minimized ? "Minimized\n" : "Client size changed\n");
        } else if (message == WM_MOVE) {
            if (state->hud) state->hud->updateLayout();
        } else if (message == WM_SETFOCUS || message == WM_KILLFOCUS) {
            state->focused = message == WM_SETFOCUS;
            state->timer.reset();
            if (!state->focused) {
                state->input.clear();
                if (state->captured) ReleaseCapture();
            }
            *state->log << (state->focused ? "Focus gained\n" : "Focus lost\n");
        } else if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN) {
            state->input.keyDown(static_cast<std::size_t>(wParam));
            if (wParam == VK_ESCAPE && state->captured) ReleaseCapture();
        } else if (message == WM_KEYUP || message == WM_SYSKEYUP) {
            state->input.keyUp(static_cast<std::size_t>(wParam));
        } else if (message == WM_RBUTTONDOWN) {
            state->input.discardMouse();
            SetCapture(window);
            state->captured = GetCapture() == window;
            state->input.keyDown(VK_RBUTTON);
        } else if (message == WM_RBUTTONUP) {
            state->input.keyUp(VK_RBUTTON);
        } else if (message == WM_CAPTURECHANGED) {
            state->captured = false;
            state->input.keyUp(VK_RBUTTON);
            state->input.discardMouse();
        } else if (message == WM_INPUT && state->captured && state->focused) {
            RAWINPUT raw{};
            UINT size = sizeof(raw);
            if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &raw, &size,
                                sizeof(RAWINPUTHEADER)) == sizeof(raw) &&
                raw.header.dwType == RIM_TYPEMOUSE && !(raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE))
                state->input.addMouse(raw.data.mouse.lLastX, raw.data.mouse.lLastY);
        } else if (message == WM_DPICHANGED) {
            const auto* suggested = reinterpret_cast<RECT*>(lParam);
            SetWindowPos(window, nullptr, suggested->left, suggested->top,
                suggested->right - suggested->left, suggested->bottom - suggested->top,
                SWP_NOACTIVATE | SWP_NOZORDER);
            if (state->hud) state->hud->updateLayout();
            *state->log << "DPI changed: " << HIWORD(wParam) << '\n';
            return 0;
        }
    }
    if (message == WM_CLOSE) { PostQuitMessage(0); return 0; }
    if (message == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(window, message, wParam, lParam);
}
}
int main(int argc, char** argv) {
    wchar_t executable[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, executable, MAX_PATH);
    if (!length || length == MAX_PATH) { std::cerr << "Cannot locate executable\n"; return 1; }
    std::ofstream log(std::filesystem::path(executable).parent_path() / "dx11-diagnostics.log", std::ios::trunc);
    if (!log) { std::cerr << "Cannot open diagnostics file next to executable\n"; return 1; }
    try {
        unsigned long frames = 0;
        bool lifecycleTest = false;
        bool reverseOrder = false;
        bool depthCheck = false;
        bool useDx12 = false, clearOnly = false, referenceScene = false, demo = false;
        std::filesystem::path capturePath;
        std::filesystem::path benchmarkPath;
        std::uint32_t seed = AsteroidField::defaultSeed;
        for (int i = 1; i < argc; ++i) {
            const std::string arg(argv[i]);
            if (arg == "--backend" && i + 1 < argc) {
                const std::string value(argv[++i]);
                if (value != "dx11" && value != "dx12") throw std::runtime_error("Backend must be dx11 or dx12");
                useDx12 = value == "dx12";
            } else if (arg == "--scene" && i + 1 < argc) {
                const std::string value(argv[++i]);
                if (value != "reference" && value != "sandbox") throw std::runtime_error("Scene must be reference or sandbox");
                referenceScene = value == "reference";
            } else if (arg == "--capture" && i + 1 < argc) capturePath = argv[++i];
            else if (arg == "--benchmark" && i + 1 < argc) benchmarkPath = argv[++i];
            else if (arg == "--seed" && i + 1 < argc) {
                const std::string value(argv[++i]); size_t parsed = 0;
                const auto number = std::stoull(value, &parsed, 10);
                if (value.empty() || value[0] == '-' || parsed != value.size() || number > UINT32_MAX)
                    throw std::runtime_error("Seed must be a decimal integer from 0 to 4294967295");
                seed = static_cast<std::uint32_t>(number);
            } else if (arg == "--demo") demo = true;
            else if (arg == "--clear-only") clearOnly = true;
            else if (arg == "--lifecycle-test") lifecycleTest = true;
            else if (arg == "--reverse-order") reverseOrder = true;
            else if (arg == "--depth-check") { depthCheck = true; frames = 1; }
            else if (arg == "--frames" && i + 1 < argc) {
                const std::string value(argv[++i]); size_t parsed = 0;
                frames = std::stoul(value, &parsed);
                if (parsed != value.size() || frames == 0 || value[0] == '-') throw std::runtime_error("Frame count must be a positive integer");
            } else throw std::runtime_error("Usage: starfall [--backend dx11|dx12] [--scene reference|sandbox] [--demo] [--seed uint32] [--capture path] [--clear-only] [--frames count] [--lifecycle-test] [--reverse-order] [--depth-check]");
        }
        if (!benchmarkPath.empty()) {
            if (demo || depthCheck || clearOnly || lifecycleTest || reverseOrder || frames || !capturePath.empty())
                throw std::runtime_error("Benchmark cannot be combined with diagnostic or scene options");
            referenceScene = true;
        }
        if (depthCheck) referenceScene = true;
        if (demo && referenceScene) throw std::runtime_error("--demo requires sandbox scene");
        if (depthCheck && useDx12) throw std::runtime_error("--depth-check is only supported by DX11");
        if (clearOnly && !useDx12) throw std::runtime_error("--clear-only requires DX12");
        log << "Backend: " << (useDx12 ? "dx12" : "dx11") << "; scene: " << (referenceScene ? "reference" : "sandbox") << (demo ? "; demo" : "") << '\n';
        if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
            throw std::runtime_error("SetProcessDpiAwarenessContext failed: " + std::to_string(GetLastError()));
        log << "DPI awareness: per-monitor v2\n";
        const HINSTANCE instance = GetModuleHandleW(nullptr);
        WNDCLASSW wc{};
        wc.lpfnWndProc = windowProcedure;
        wc.hInstance = instance;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.lpszClassName = L"StarfallWindow";
        if (!RegisterClassW(&wc)) throw std::runtime_error("RegisterClassW failed: " + std::to_string(GetLastError()));
        const DWORD style = benchmarkPath.empty() ? WS_OVERLAPPEDWINDOW : WS_POPUP;
        RECT rect{0, 0, benchmarkPath.empty() ? 1280L : 1920L, benchmarkPath.empty() ? 720L : 1080L};
        if (!AdjustWindowRectExForDpi(&rect, style, FALSE, 0, GetDpiForSystem()))
            throw std::runtime_error("AdjustWindowRectExForDpi failed");
        WindowState state;
        state.log = &log;
        HWND window = CreateWindowExW(0, wc.lpszClassName, L"Starfall", style,
            CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top,
            nullptr, nullptr, instance, &state);
        if (!window) throw std::runtime_error("CreateWindowExW failed: " + std::to_string(GetLastError()));
        try {
            RAWINPUTDEVICE mouse{0x01, 0x02, 0, window};
            if (!RegisterRawInputDevices(&mouse, 1, sizeof(mouse)))
                throw std::runtime_error("RegisterRawInputDevices failed: " + std::to_string(GetLastError()));
            Renderer graphics(log, useDx12, clearOnly);
            RECT client{};
            if (!GetClientRect(window, &client)) throw std::runtime_error("GetClientRect failed");
            graphics.initialize(window, client.right - client.left, client.bottom - client.top);
            if (!benchmarkPath.empty()) {
                if (client.right != 1920 || client.bottom != 1080) throw std::runtime_error("Benchmark client is not 1920x1080");
                graphics.setVsync(false); graphics.enableTiming();
                Microsoft::WRL::ComPtr<IDXGIFactory1> factory;
                if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
                    for (UINT i = 0;; ++i) {
                        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
                        if (factory->EnumAdapters1(i, &adapter) == DXGI_ERROR_NOT_FOUND) break;
                        DXGI_ADAPTER_DESC1 desc{}; adapter->GetDesc1(&desc);
                        if (desc.VendorId != 0x10de || (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) continue;
                        log << "Dedicated physical VRAM bytes: " << desc.DedicatedVideoMemory << '\n';
                        Microsoft::WRL::ComPtr<IDXGIAdapter3> adapter3;
                        DXGI_QUERY_VIDEO_MEMORY_INFO info{};
                        if (SUCCEEDED(adapter.As(&adapter3)) && SUCCEEDED(adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info)))
                            log << "Local memory budget bytes: " << info.Budget << "; process usage bytes: " << info.CurrentUsage << '\n';
                        else log << "Local memory budget/usage unavailable\n";
                        break;
                    }
                }
            }
            if (!capturePath.empty()) graphics.captureNext(capturePath);
            state.sizeChanged = false;
            ShowWindow(window, SW_SHOW);
            UpdateWindow(window);
            Camera camera;
            Ship ship;
            AsteroidField field(seed);
            auto fieldObjects = field.objects();
            unsigned long contacts = 0;
            std::unique_ptr<Hud> hud;
            if (!referenceScene) hud = std::make_unique<Hud>(window);
            state.hud = hud.get();
            struct HudStateGuard { WindowState& state; ~HudStateGuard() { state.hud = nullptr; } } hudStateGuard{state};
            double hudElapsed = 1.0;
            log << "Asteroid seed: " << seed << "; rocks: " << field.count << '\n';
            bool chase = !referenceScene;
            auto objects = scene::referenceObjects;
            if (reverseOrder) std::reverse(objects.begin(), objects.end());
            log << "Reference draw order: " << (reverseOrder ? "reversed" : "normal") << '\n';
            bool running = true;
            unsigned long presented = 0;
            unsigned testStage = 0;
            ULONGLONG cpuStart = 0;
            auto cpuTime = []() {
                FILETIME created{}, exited{}, kernel{}, user{};
                if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user))
                    throw std::runtime_error("GetProcessTimes failed");
                ULARGE_INTEGER k{}, u{};
                k.LowPart = kernel.dwLowDateTime; k.HighPart = kernel.dwHighDateTime;
                u.LowPart = user.dwLowDateTime; u.HighPart = user.dwHighDateTime;
                return k.QuadPart + u.QuadPart;
            };
            if (lifecycleTest && !SetTimer(window, 1, 100, nullptr))
                throw std::runtime_error("SetTimer failed");
            double submissionMs = 0.0;
            struct Sample { double frame, update, submit; std::optional<double> gpu; bool measured; };
            std::vector<Sample> samples;
            auto benchmarkStart = FrameTimer::Clock::now();
            auto previousStart = benchmarkStart;
            while (running) {
                const auto updateStart = FrameTimer::Clock::now();
                state.input.beginFrame();
                MSG message{};
                while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
                    if (message.message == WM_QUIT) { running = false; break; }
                    if (lifecycleTest && message.message == WM_TIMER && message.wParam == 1) {
                        if (testStage < 4) {
                            SetWindowPos(window, nullptr, 0, 0, 800 + 100 * testStage,
                                600 + 50 * testStage, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
                        } else if (testStage == 4) {
                            ShowWindow(window, SW_MINIMIZE);
                            cpuStart = cpuTime();
                            if (!SetTimer(window, 1, 500, nullptr)) throw std::runtime_error("SetTimer failed");
                        } else if (testStage == 5) {
                            log << "Minimized process CPU over 500 ms: " << (cpuTime() - cpuStart) / 10000.0 << " ms\n";
                            ShowWindow(window, SW_RESTORE);
                            if (!SetTimer(window, 1, 100, nullptr)) throw std::runtime_error("SetTimer failed");
                        } else if (testStage == 6) {
                            ShowWindow(window, SW_MINIMIZE);
                        } else {
                            KillTimer(window, 1);
                            PostMessageW(window, WM_CLOSE, 0, 0);
                            log << "Lifecycle test: requested close while minimized\n";
                        }
                        ++testStage;
                        continue;
                    }
                    TranslateMessage(&message);
                    DispatchMessageW(&message);
                }
                if (!running) break;
                if (hud) hud->updateLayout();
                if (state.minimized || (!state.focused && benchmarkPath.empty())) {
                    state.timer.reset();
                    if (!WaitMessage()) throw std::runtime_error("WaitMessage failed");
                    continue;
                }
                if (state.sizeChanged) {
                    if (!GetClientRect(window, &client)) throw std::runtime_error("GetClientRect failed");
                    const auto width = client.right - client.left;
                    const auto height = client.bottom - client.top;
                    if (!benchmarkPath.empty() && (width != 1920 || height != 1080)) throw std::runtime_error("Benchmark viewport changed");
                    if (!width || !height) {
                        if (!WaitMessage()) throw std::runtime_error("WaitMessage failed");
                        continue;
                    }
                    graphics.resize(width, height);
                    state.sizeChanged = false;
                }
                const double dt = state.timer.tick(FrameTimer::Clock::now());
                if (state.input.pressed('C') && !referenceScene) {
                    chase = !chase;
                    state.input.discardMouse();
                }
                if (state.input.pressed('R')) {
                    state.input.discardMouse();
                    camera.reset();
                    ship.reset();
                    field.reset(seed);
                    fieldObjects = field.objects();
                    contacts = 0;
                } else if (chase) {
                    if (!demo && state.captured) ship.look(static_cast<float>(state.input.mouseX()), static_cast<float>(state.input.mouseY()));
                    const float step = demo ? 1.0f / 60.0f : static_cast<float>(dt);
                    const auto previousPosition = ship.position();
                    ship.update(demo ? 1.0f : static_cast<float>(state.input.held('W')) - static_cast<float>(state.input.held('S')),
                        demo ? 0.35f : static_cast<float>(state.input.held('D')) - static_cast<float>(state.input.held('A')),
                        demo ? 0.12f : static_cast<float>(state.input.held(VK_UP)) - static_cast<float>(state.input.held(VK_DOWN)),
                        demo ? 0.0f : static_cast<float>(state.input.held(VK_SPACE)) - static_cast<float>(state.input.held(VK_CONTROL)),
                        !demo && state.input.held(VK_SHIFT), step);
                    if (field.contact(ship.objects(), previousPosition, ship.position())) {
                        ++contacts;
                        ship.reset();
                        log << "Asteroid contact " << contacts << "; ship reset\n";
                    }
                } else {
                    if (state.captured) camera.look(static_cast<float>(state.input.mouseX()), static_cast<float>(state.input.mouseY()));
                    camera.move(static_cast<float>(state.input.held('D')) - static_cast<float>(state.input.held('A')),
                        static_cast<float>(state.input.held('E')) - static_cast<float>(state.input.held('Q')),
                        static_cast<float>(state.input.held('W')) - static_cast<float>(state.input.held('S')),
                        static_cast<float>(dt), state.input.held(VK_SHIFT));
                }
                hudElapsed += dt;
                if (hud && hudElapsed >= 0.25) {
                    std::wostringstream overlay;
                    overlay << L"STARFALL / FLIGHT SANDBOX\n"
                        << L"Mouse steer   W/S thrust   A/D yaw   Up/Down pitch   Space/Ctrl rise/drop\n"
                        << L"S reverse/brake   Shift boost   C camera   R reset   Esc release mouse\n"
                        << L"Free camera: right-click + mouse, WASD + Q/E\n"
                        << (chase ? L"CHASE" : L"FREE") << L"   Seed " << seed << L"   Contacts " << contacts;
                    hud->setText(overlay.str());
                    hudElapsed = 0;
                }
                const auto updateEnd = FrameTimer::Clock::now();
                const double updateMs = std::chrono::duration<double, std::milli>(updateEnd - updateStart).count();
                if (benchmarkPath.empty()) {
                auto keys = [&](auto predicate) {
                    std::wostringstream result;
                    bool any = false;
                    for (std::size_t key = 0; key < InputState::keyCount; ++key) {
                        if (predicate(key)) { if (any) result << L","; result << key; any = true; }
                    }
                    return any ? result.str() : std::wstring(L"-");
                };
                std::wostringstream title;
                title << (referenceScene ? L"Reference" : (chase ? L"Flight (C: free, R: reset)" : L"Free (C: chase, R: reset)")) << L" | "
                      << std::fixed << std::setprecision(2)
                      << L"Starfall | seed " << seed << L" | contacts " << contacts << L" | dt " << dt * 1000.0 << L" ms | update " << updateMs
                      << L" ms | submit/present " << submissionMs << L" ms | held "
                      << keys([&](auto key) { return state.input.held(key); }) << L" | press "
                      << keys([&](auto key) { return state.input.pressed(key); }) << L" | release "
                      << keys([&](auto key) { return state.input.released(key); }) << L" | mouse "
                      << state.input.mouseX() << L"," << state.input.mouseY()
                      << (state.captured ? L" | captured (Esc releases)" : L" | right-click to capture");
                SetWindowTextW(window, title.str().c_str());
                }
                const auto submitStart = FrameTimer::Clock::now();
                if (referenceScene) {
                    graphics.present(objects, benchmarkPath.empty() ? camera.view() : scene::view(), depthCheck);
                } else {
                    auto sandboxObjects = ship.objects();
                    sandboxObjects.insert(sandboxObjects.end(), fieldObjects.begin(), fieldObjects.end());
                    graphics.present(sandboxObjects, chase ? ship.chaseView() : camera.view(), depthCheck);
                }
                submissionMs = std::chrono::duration<double, std::milli>(FrameTimer::Clock::now() - submitStart).count();
                if (!benchmarkPath.empty()) {
                    const double elapsed = std::chrono::duration<double>(updateStart - benchmarkStart).count();
                    samples.push_back({std::chrono::duration<double, std::milli>(updateStart - previousStart).count(), updateMs, submissionMs, {}, elapsed >= 2.0 && elapsed < 7.0});
                    previousStart = updateStart;
                    for (const auto& result : graphics.takeTimings())
                        if (result.frame < samples.size() && result.valid) samples[static_cast<size_t>(result.frame)].gpu = result.milliseconds;
                    if (elapsed >= 7.0) running = false;
                }
                ++presented;
                if (frames && presented >= frames) running = false;
            }
            if (!benchmarkPath.empty()) {
                for (const auto& result : graphics.takeTimings())
                    if (result.frame < samples.size() && result.valid) samples[static_cast<size_t>(result.frame)].gpu = result.milliseconds;
                if (!benchmarkPath.parent_path().empty()) std::filesystem::create_directories(benchmarkPath.parent_path());
                std::ofstream csv(benchmarkPath); csv << "frame,frame_ms,update_ms,submission_ms,gpu_ms\n" << std::setprecision(10);
                for (size_t i = 0; i < samples.size(); ++i) if (samples[i].measured) {
                    const auto& s = samples[i]; csv << i << ',' << s.frame << ',' << s.update << ',' << s.submit << ',';
                    if (s.gpu) csv << *s.gpu;
                    csv << '\n';
                }
                if (!csv) throw std::runtime_error("Cannot write benchmark CSV");
                log << "Benchmark raw CSV: " << benchmarkPath.string() << '\n';
            }
            log << "Presented frames: " << presented << '\n';
        } catch (...) { DestroyWindow(window); UnregisterClassW(wc.lpszClassName, instance); throw; }
        if (IsWindow(window)) DestroyWindow(window);
        UnregisterClassW(wc.lpszClassName, instance);
        log << "Clean shutdown\n";
        return 0;
    } catch (const std::exception& error) {
        log << "Error: " << error.what() << '\n';
        std::cerr << error.what() << '\n';
        return 1;
    }
}
