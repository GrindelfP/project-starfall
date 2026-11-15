#pragma once
#include "dx11.h"
#include "dx12.h"
#include <memory>
#include <stdexcept>
class Renderer {
public:
    Renderer(std::ostream& log, bool dx12, bool clearOnly) : clearOnly_(clearOnly) {
        if (dx12) dx12_ = std::make_unique<Dx12>(log); else dx11_ = std::make_unique<Dx11>(log);
    }
    void initialize(HWND window, unsigned w, unsigned h) { if (dx12_) dx12_->initialize(window,w,h); else dx11_->initialize(window,w,h); }
    void resize(unsigned w, unsigned h) { if (dx12_) dx12_->resize(w,h); else dx11_->resize(w,h); }
    void present(std::span<const scene::Object> objects, DirectX::FXMMATRIX view, bool depthCheck) {
        if (dx12_) dx12_->present(objects, view, clearOnly_); else dx11_->present(objects, view, depthCheck);
    }
private:
    std::unique_ptr<Dx11> dx11_;
    std::unique_ptr<Dx12> dx12_;
    bool clearOnly_;
};
