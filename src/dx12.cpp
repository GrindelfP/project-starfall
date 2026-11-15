#include "dx12.h"
#include <d3d12sdklayers.h>
#include <dxgidebug.h>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <vector>
using Microsoft::WRL::ComPtr;
namespace {
void check(HRESULT hr, const char* what) {
    if (FAILED(hr)) { std::ostringstream s; s << what << " failed (HRESULT 0x" << std::hex << static_cast<unsigned long>(hr) << ")"; throw std::runtime_error(s.str()); }
}
}
void Dx12::initialize(HWND window, unsigned width, unsigned height) {
    UINT flags = 0;
#ifdef _DEBUG
    ComPtr<ID3D12Debug> debug;
    check(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)), "D3D12GetDebugInterface (install Windows Graphics Tools)");
    debug->EnableDebugLayer();
    ComPtr<ID3D12Debug1> debug1;
    check(debug.As(&debug1), "ID3D12Debug1");
    debug1->SetEnableGPUBasedValidation(TRUE);
    flags = DXGI_CREATE_FACTORY_DEBUG;
    log_ << "DX12 debug layer and GPU-based validation enabled\n";
#endif
    ComPtr<IDXGIFactory4> factory;
    check(CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory)), "CreateDXGIFactory2");
    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0;; ++i) {
        ComPtr<IDXGIAdapter1> candidate;
        HRESULT hr = factory->EnumAdapters1(i, &candidate);
        if (hr == DXGI_ERROR_NOT_FOUND) break;
        check(hr, "EnumAdapters1");
        DXGI_ADAPTER_DESC1 desc{}; check(candidate->GetDesc1(&desc), "GetDesc1");
        if (desc.VendorId == 0x10de && !(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) &&
            SUCCEEDED(D3D12CreateDevice(candidate.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr))) {
            adapter = candidate;
            char name[256]{}; WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, name, sizeof(name), nullptr, nullptr);
            log_ << "Adapter: " << name << " (vendor 0x" << std::hex << desc.VendorId << ", device 0x" << desc.DeviceId << std::dec << ")\n";
            break;
        }
    }
    if (!adapter) throw std::runtime_error("No DirectX 12 capable NVIDIA hardware adapter found");
    check(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device_)), "D3D12CreateDevice");
    D3D12_COMMAND_QUEUE_DESC q{}; q.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    check(device_->CreateCommandQueue(&q, IID_PPV_ARGS(&queue_)), "CreateCommandQueue");
    DXGI_SWAP_CHAIN_DESC1 s{}; s.Width = width; s.Height = height; s.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    s.SampleDesc.Count = 1; s.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; s.BufferCount = 2; s.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    ComPtr<IDXGISwapChain1> swap;
    check(factory->CreateSwapChainForHwnd(queue_.Get(), window, &s, nullptr, nullptr, &swap), "CreateSwapChainForHwnd");
    check(swap.As(&swap_), "IDXGISwapChain3");
    check(factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER), "MakeWindowAssociation");
    D3D12_DESCRIPTOR_HEAP_DESC heap{}; heap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; heap.NumDescriptors = 2;
    check(device_->CreateDescriptorHeap(&heap, IID_PPV_ARGS(&rtvHeap_)), "Create RTV heap");
    rtvStride_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    check(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator_)), "CreateCommandAllocator");
    check(device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator_.Get(), nullptr, IID_PPV_ARGS(&commands_)), "CreateCommandList");
    check(commands_->Close(), "Close initial command list");
    check(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)), "CreateFence");
    event_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!event_) throw std::runtime_error("CreateEvent failed");
    createTargets(width, height);
}
void Dx12::createTargets(unsigned width, unsigned height) {
    auto handle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < 2; ++i) {
        check(swap_->GetBuffer(i, IID_PPV_ARGS(&targets_[i])), "GetBuffer");
        device_->CreateRenderTargetView(targets_[i].Get(), nullptr, handle); handle.ptr += rtvStride_;
    }
    viewport_ = {0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1};
    scissor_ = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    DirectX::XMStoreFloat4x4(&projection_, scene::projection(width, height));
    log_ << "Client/viewport: " << width << " x " << height << '\n';
}
void Dx12::wait() {
    const auto value = ++fenceValue_;
    check(queue_->Signal(fence_.Get(), value), "Signal fence");
    if (fence_->GetCompletedValue() < value) {
        check(fence_->SetEventOnCompletion(value, event_), "SetEventOnCompletion");
        if (WaitForSingleObject(event_, INFINITE) != WAIT_OBJECT_0) throw std::runtime_error("Fence wait failed");
    }
    if (fence_->GetCompletedValue() == UINT64_MAX) check(device_->GetDeviceRemovedReason(), "Device removed");
}
void Dx12::resize(unsigned width, unsigned height) {
    if (!width || !height) return;
    wait(); for (auto& target : targets_) target.Reset(); depth_.Reset();
    check(swap_->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0), "ResizeBuffers");
    createTargets(width, height);
}
void Dx12::present(std::span<const scene::Object>, DirectX::FXMMATRIX, bool) {
    check(allocator_->Reset(), "Reset allocator");
    check(commands_->Reset(allocator_.Get(), nullptr), "Reset command list");
    const UINT frame = swap_->GetCurrentBackBufferIndex();
    D3D12_RESOURCE_BARRIER barrier{}; barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = targets_[frame].Get(); barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT; barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    commands_->ResourceBarrier(1, &barrier);
    auto rtv = rtvHeap_->GetCPUDescriptorHandleForHeapStart(); rtv.ptr += frame * rtvStride_;
    constexpr float clear[] = {0.025f, 0.12f, 0.25f, 1.0f}; commands_->ClearRenderTargetView(rtv, clear, 0, nullptr);
    std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter); commands_->ResourceBarrier(1, &barrier);
    check(commands_->Close(), "Close command list"); ID3D12CommandList* lists[] = {commands_.Get()}; queue_->ExecuteCommandLists(1, lists);
    check(swap_->Present(1, 0), "Present"); wait();
}
void Dx12::dumpMessages() {
#ifdef _DEBUG
    ComPtr<ID3D12InfoQueue> info;
    if (!device_ || FAILED(device_.As(&info))) return;
    for (UINT64 i = 0; i < info->GetNumStoredMessages(); ++i) {
        SIZE_T size = 0; if (FAILED(info->GetMessage(i, nullptr, &size))) continue;
        std::vector<char> data(size); auto* m = reinterpret_cast<D3D12_MESSAGE*>(data.data());
        if (SUCCEEDED(info->GetMessage(i, m, &size))) log_ << "[severity " << m->Severity << "] " << m->pDescription << '\n';
    }
    info->ClearStoredMessages();
#endif
}
Dx12::~Dx12() {
    try { if (queue_ && fence_ && event_) wait(); } catch (const std::exception& e) { log_ << "Shutdown wait: " << e.what() << '\n'; }
    for (auto& target : targets_) target.Reset(); depth_.Reset(); vertices_.Reset(); indices_.Reset();
    pipeline_.Reset(); root_.Reset(); commands_.Reset(); allocator_.Reset(); rtvHeap_.Reset(); dsvHeap_.Reset(); swap_.Reset(); fence_.Reset(); queue_.Reset();
#ifdef _DEBUG
    if (device_) {
        dumpMessages(); ComPtr<ID3D12DebugDevice> debug;
        if (SUCCEEDED(device_.As(&debug))) { log_ << "Live objects (device and diagnostics remain):\n"; debug->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL); dumpMessages(); }
    }
#endif
    device_.Reset(); if (event_) CloseHandle(event_);
}
