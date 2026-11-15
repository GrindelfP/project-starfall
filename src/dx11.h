#pragma once
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <ostream>
#include <span>
#include <filesystem>
#include <array>
#include <vector>
#include "gpu_sample.h"
#include "scene.h"

// Owns all graphics resources; the HWND must outlive this object.
class Dx11 final {
public:
    explicit Dx11(std::ostream& log) : log_(log) {}
    ~Dx11();
    void initialize(HWND window, unsigned width, unsigned height);
    void resize(unsigned width, unsigned height);
    void enableTiming();
    std::vector<GpuSample> takeTimings();
    void setVsync(bool enabled) { vsync_ = enabled; }
    void present(std::span<const scene::Object> objects, DirectX::FXMMATRIX view, bool verifyOrder = false);
    // Development capture: one synchronous readback, never enabled during timing runs.
    void captureNext(const std::filesystem::path& path) { capturePath_ = path; }
private:
    struct QuerySlot { Microsoft::WRL::ComPtr<ID3D11Query> disjoint, start, end; unsigned long long frame = 0; bool pending = false; };
    std::array<QuerySlot, 64> queries_{};
    std::vector<GpuSample> samples_;
    unsigned long long timingFrame_ = 0;
    bool timing_ = false, vsync_ = true;
    void pollTimings();
    void saveCapture();
    std::filesystem::path capturePath_;
    void checkDevice(HRESULT result, const char* operation);
    void createTarget(unsigned width, unsigned height);
    void createScene();
    std::ostream& log_;
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> target_;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthTarget_;
    DirectX::XMFLOAT4X4 projection_{};
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader_;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer_, indexBuffer_, constants_;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizer_;
    Microsoft::WRL::ComPtr<ID3D11BlendState> blend_;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depth_;
};
