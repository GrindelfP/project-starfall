#pragma once
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <ostream>
#include <span>
#include <filesystem>
#include "scene.h"

class Dx12 final {
public:
    explicit Dx12(std::ostream& log) : log_(log) {}
    ~Dx12();
    void initialize(HWND window, unsigned width, unsigned height);
    void resize(unsigned width, unsigned height);
    void captureNext(const std::filesystem::path& path) { capturePath_ = path; }
    void present(std::span<const scene::Object> objects, DirectX::FXMMATRIX view, bool clearOnly = false);
private:
    void wait();
    void createTargets(unsigned width, unsigned height);
    void createScene();
    void dumpMessages();
    std::filesystem::path capturePath_;
    std::ostream& log_;
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue_;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> swap_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_, dsvHeap_;
    Microsoft::WRL::ComPtr<ID3D12Resource> targets_[2], depth_, vertices_, indices_;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator_;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commands_;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_;
    HANDLE event_ = nullptr;
    UINT64 fenceValue_ = 0;
    UINT rtvStride_ = 0;
    D3D12_VIEWPORT viewport_{};
    D3D12_RECT scissor_{};
    DirectX::XMFLOAT4X4 projection_{};
};

