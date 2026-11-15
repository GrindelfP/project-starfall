#include "dx11.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
struct WindowState {
    bool minimized = false;
    bool sizeChanged = true;
    std::ostream* log = nullptr;
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
            *state->log << (state->minimized ? "Minimized\n" : "Client size changed\n");
        } else if (message == WM_SETFOCUS || message == WM_KILLFOCUS) {
            *state->log << (message == WM_SETFOCUS ? "Focus gained\n" : "Focus lost\n");
        } else if (message == WM_DPICHANGED) {
            const auto* suggested = reinterpret_cast<RECT*>(lParam);
            SetWindowPos(window, nullptr, suggested->left, suggested->top,
                suggested->right - suggested->left, suggested->bottom - suggested->top,
                SWP_NOACTIVATE | SWP_NOZORDER);
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
        if (argc != 1) {
            lifecycleTest = argc == 2 && std::string(argv[1]) == "--lifecycle-test";
            if (!lifecycleTest) {
                if (argc != 3 || std::string(argv[1]) != "--frames")
                    throw std::runtime_error("Usage: starfall [--frames positive-count | --lifecycle-test]");
                const std::string value(argv[2]);
                size_t parsed = 0;
                frames = std::stoul(value, &parsed);
                if (parsed != value.size() || frames == 0 || value[0] == '-')
                    throw std::runtime_error("Frame count must be a positive integer");
            }
        }
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
        constexpr DWORD style = WS_OVERLAPPEDWINDOW;
        RECT rect{0, 0, 1280, 720};
        if (!AdjustWindowRectExForDpi(&rect, style, FALSE, 0, GetDpiForSystem()))
            throw std::runtime_error("AdjustWindowRectExForDpi failed");
        WindowState state{false, true, &log};
        HWND window = CreateWindowExW(0, wc.lpszClassName, L"Starfall - DirectX 11", style,
            CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top,
            nullptr, nullptr, instance, &state);
        if (!window) throw std::runtime_error("CreateWindowExW failed: " + std::to_string(GetLastError()));
        try {
            Dx11 graphics(log);
            RECT client{};
            if (!GetClientRect(window, &client)) throw std::runtime_error("GetClientRect failed");
            graphics.initialize(window, client.right - client.left, client.bottom - client.top);
            state.sizeChanged = false;
            ShowWindow(window, SW_SHOW);
            UpdateWindow(window);
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
            while (running) {
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
                if (state.minimized) {
                    if (!WaitMessage()) throw std::runtime_error("WaitMessage failed");
                    continue;
                }
                if (state.sizeChanged) {
                    if (!GetClientRect(window, &client)) throw std::runtime_error("GetClientRect failed");
                    const auto width = client.right - client.left;
                    const auto height = client.bottom - client.top;
                    if (!width || !height) {
                        if (!WaitMessage()) throw std::runtime_error("WaitMessage failed");
                        continue;
                    }
                    graphics.resize(width, height);
                    state.sizeChanged = false;
                }
                graphics.present();
                ++presented;
                if (frames && presented >= frames) running = false;
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


