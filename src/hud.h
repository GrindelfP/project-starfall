#pragma once
#include <windows.h>
#include <algorithm>
#include <stdexcept>
#include <string>

// A small native, click-through help panel. Hidden in the reference/benchmark scene.
// It owns only GDI/Win32 objects and does not enter either renderer's GPU workload.
class Hud final {
public:
    explicit Hud(HWND parent) : parent_(parent), dpi_(GetDpiForWindow(parent)) {
        WNDCLASSW wc{};
        wc.lpfnWndProc = procedure;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"StarfallHelpPanel";
        if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            throw std::runtime_error("Cannot register the Starfall help panel");
        createFonts();
        window_ = CreateWindowExW(WS_EX_NOACTIVATE, wc.lpszClassName, L"Starfall controls",
            WS_CHILD | WS_VISIBLE, px(18), px(18), px(500), px(146), parent, nullptr, wc.hInstance, this);
        if (!window_) {
            DeleteObject(titleFont_); DeleteObject(bodyFont_);
            throw std::runtime_error("Cannot create the Starfall help panel");
        }
    }
    Hud(const Hud&) = delete;
    Hud& operator=(const Hud&) = delete;
    ~Hud() {
        if (IsWindow(window_)) DestroyWindow(window_);
        DeleteObject(titleFont_); DeleteObject(bodyFont_);
    }
    void setText(const std::wstring& text) {
        const UINT currentDpi = GetDpiForWindow(parent_);
        const bool dpiChanged = currentDpi && currentDpi != dpi_;
        if (dpiChanged) {
            dpi_ = currentDpi;
            DeleteObject(titleFont_); DeleteObject(bodyFont_);
            createFonts();
        }
        text_ = text;
        RECT client{};
        GetClientRect(parent_, &client);
        const int x = (std::min)(px(18), static_cast<int>(client.right));
        const int y = (std::min)(px(18), static_cast<int>(client.bottom));
        const int width = (std::max)(0, (std::min)(px(500), static_cast<int>(client.right) - x));
        const int height = (std::max)(0, (std::min)(px(146), static_cast<int>(client.bottom) - y));
        SetWindowPos(window_, HWND_TOP, x, y, width, height, SWP_NOACTIVATE);
        InvalidateRect(window_, nullptr, FALSE);
    }
private:
    int px(int value) const { return MulDiv(value, static_cast<int>(dpi_), 96); }
    void createFonts() {
        titleFont_ = CreateFontW(-px(19), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH, L"Segoe UI");
        bodyFont_ = CreateFontW(-px(13), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH, L"Segoe UI");
    }
    static LRESULT CALLBACK procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
        auto* self = reinterpret_cast<Hud*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            self = static_cast<Hud*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (message == WM_NCHITTEST) return HTTRANSPARENT;
        if (message == WM_MOUSEACTIVATE) return MA_NOACTIVATE;
        if (message == WM_ERASEBKGND) return 1;
        if (message == WM_PAINT && self) {
            PAINTSTRUCT paint{};
            HDC dc = BeginPaint(window, &paint);
            RECT bounds{}; GetClientRect(window, &bounds);
            HBRUSH background = CreateSolidBrush(RGB(5, 14, 29));
            FillRect(dc, &bounds, background); DeleteObject(background);
            RECT accent{0, 0, self->px(3), bounds.bottom};
            HBRUSH cyan = CreateSolidBrush(RGB(57, 208, 230));
            FillRect(dc, &accent, cyan); DeleteObject(cyan);
            SetBkMode(dc, TRANSPARENT);
            const auto newline = self->text_.find(L'\n');
            const auto heading = self->text_.substr(0, newline);
            const auto body = newline == std::wstring::npos ? std::wstring{} : self->text_.substr(newline + 1);
            RECT title{self->px(16), self->px(10), bounds.right-self->px(12), self->px(38)};
            HGDIOBJ old = SelectObject(dc, self->titleFont_);
            SetTextColor(dc, RGB(108, 226, 240));
            DrawTextW(dc, heading.c_str(), -1, &title, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
            SelectObject(dc, self->bodyFont_);
            SetTextColor(dc, RGB(213, 225, 239));
            RECT content{self->px(16), self->px(43), bounds.right-self->px(12), bounds.bottom-self->px(8)};
            DrawTextW(dc, body.c_str(), -1, &content, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);
            SelectObject(dc, old);
            EndPaint(window, &paint);
            return 0;
        }
        return DefWindowProcW(window, message, wParam, lParam);
    }
    HWND parent_{}, window_{};
    UINT dpi_ = 96;
    HFONT titleFont_{}, bodyFont_{};
    std::wstring text_;
};
