#pragma once
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <ostream>

// Owns all graphics resources; the HWND must outlive this object.
class Dx11 final {
public:
    explicit Dx11(std::ostream& log) : log_(log) {}
    ~Dx11();
    void initialize(HWND window, unsigned width, unsigned height);
    void resize(unsigned width, unsigned height);
    void present();
private:
    void checkDevice(HRESULT result, const char* operation);
    void createTarget(unsigned width, unsigned height);
    std::ostream& log_;
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> target_;
};
