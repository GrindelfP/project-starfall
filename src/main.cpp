#include "dx11.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_CLOSE) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(window, message, wParam, lParam);
}
}
int main(int argc, char** argv) {
    std::filesystem::create_directories("out");
    std::ofstream log("out/dx11-diagnostics.log", std::ios::trunc);
    if (!log) { std::cerr << "Cannot open out/dx11-diagnostics.log\n"; return 1; }
    try {
        unsigned long frames = 0;
        if (argc != 1) {
            if (argc != 3 || std::string(argv[1]) != "--frames")
                throw std::runtime_error("Usage: starfall [--frames positive-count]");
            const std::string value(argv[2]);
            size_t parsed = 0;
            frames = std::stoul(value, &parsed);
            if (parsed != value.size() || frames == 0 || value[0] == '-')
                throw std::runtime_error("Frame count must be a positive integer");
        }
        const HINSTANCE instance = GetModuleHandleW(nullptr);
        WNDCLASSW wc{};
        wc.lpfnWndProc = windowProcedure;
        wc.hInstance = instance;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.lpszClassName = L"StarfallWindow";
        if (!RegisterClassW(&wc)) throw std::runtime_error("RegisterClassW failed: " + std::to_string(GetLastError()));
        constexpr DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        RECT rect{0, 0, 1280, 720};
        if (!AdjustWindowRect(&rect, style, FALSE)) throw std::runtime_error("AdjustWindowRect failed");
        HWND window = CreateWindowExW(0, wc.lpszClassName, L"Starfall - DirectX 11", style,
            CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top,
            nullptr, nullptr, instance, nullptr);
        if (!window) throw std::runtime_error("CreateWindowExW failed: " + std::to_string(GetLastError()));
        try {
            Dx11 graphics(log);
            graphics.initialize(window, 1280, 720);
            ShowWindow(window, SW_SHOW);
            UpdateWindow(window);
            bool running = true;
            unsigned long presented = 0;
            while (running) {
                MSG message{};
                while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
                    if (message.message == WM_QUIT) { running = false; break; }
                    TranslateMessage(&message);
                    DispatchMessageW(&message);
                }
                if (!running) break;
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

