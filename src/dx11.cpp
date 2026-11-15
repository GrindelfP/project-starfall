#include "dx11.h"
#include <d3d11sdklayers.h>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace {
void check(HRESULT hr, const char* operation) {
    if (FAILED(hr)) {
        std::ostringstream message;
        message << operation << " failed (HRESULT 0x" << std::hex << std::uppercase
                << static_cast<unsigned long>(hr) << ")";
        throw std::runtime_error(message.str());
    }
}
const char* featureName(D3D_FEATURE_LEVEL level) {
    switch (level) {
    case D3D_FEATURE_LEVEL_12_1: return "12_1";
    case D3D_FEATURE_LEVEL_12_0: return "12_0";
    case D3D_FEATURE_LEVEL_11_1: return "11_1";
    case D3D_FEATURE_LEVEL_11_0: return "11_0";
    default: return "unexpected";
    }
}
}

void Dx11::initialize(HWND window, unsigned width, unsigned height) {
    ComPtr<IDXGIFactory1> factory;
    check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)), "CreateDXGIFactory1");
    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0;; ++i) {
        ComPtr<IDXGIAdapter1> candidate;
        const HRESULT hr = factory->EnumAdapters1(i, &candidate);
        if (hr == DXGI_ERROR_NOT_FOUND) break;
        check(hr, "EnumAdapters1");
        DXGI_ADAPTER_DESC1 desc{};
        check(candidate->GetDesc1(&desc), "GetDesc1");
        if (desc.VendorId == 0x10de && !(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
            adapter = candidate;
            char name[256]{};
            WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, name, sizeof(name), nullptr, nullptr);
            log_ << "Adapter: " << name << " (vendor 0x" << std::hex << desc.VendorId
                 << ", device 0x" << desc.DeviceId << std::dec << ")\n";
            break;
        }
    }
    if (!adapter) throw std::runtime_error("No NVIDIA hardware adapter found");

    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferDesc.Width = width;
    desc.BufferDesc.Height = height;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.OutputWindow = window;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    constexpr D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL selected{};
    check(D3D11CreateDeviceAndSwapChain(adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags,
        levels, static_cast<UINT>(std::size(levels)), D3D11_SDK_VERSION, &desc,
        &swapChain_, &device_, &selected, &context_), "D3D11CreateDeviceAndSwapChain");
    log_ << "Feature level: " << featureName(selected) << '\n';
#ifdef _DEBUG
    log_ << "DX11 debug layer enabled\n";
#endif
    check(factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER), "MakeWindowAssociation");
    ComPtr<ID3D11Texture2D> backBuffer;
    check(swapChain_->GetBuffer(0, IID_PPV_ARGS(&backBuffer)), "GetBuffer");
    check(device_->CreateRenderTargetView(backBuffer.Get(), nullptr, &target_), "CreateRenderTargetView");
}

void Dx11::present() {
    constexpr float clear[] = { 0.025f, 0.12f, 0.25f, 1.0f };
    context_->ClearRenderTargetView(target_.Get(), clear);
    check(swapChain_->Present(1, 0), "Present");
}

Dx11::~Dx11() {
    if (context_) { context_->ClearState(); context_->Flush(); }
    target_.Reset();
    swapChain_.Reset();
    context_.Reset();
#ifdef _DEBUG
    if (device_) {
        ComPtr<ID3D11Debug> debug;
        ComPtr<ID3D11InfoQueue> queue;
        if (SUCCEEDED(device_.As(&debug)) && SUCCEEDED(device_.As(&queue))) {
            log_ << "Debug messages before live-object report:\n";
            auto dump = [&]() {
                const auto count = queue->GetNumStoredMessagesAllowedByRetrievalFilter();
                for (UINT64 i = 0; i < count; ++i) {
                    SIZE_T size = 0;
                    if (FAILED(queue->GetMessage(i, nullptr, &size))) continue;
                    std::vector<char> storage(size);
                    auto* message = reinterpret_cast<D3D11_MESSAGE*>(storage.data());
                    if (SUCCEEDED(queue->GetMessage(i, message, &size)))
                        log_ << "[severity " << message->Severity << "] " << message->pDescription << '\n';
                }
                queue->ClearStoredMessages();
            };
            dump();
            log_ << "Live-object report (device and diagnostic interfaces remain during report):\n";
            debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
            dump();
        } else log_ << "Could not obtain DX11 diagnostic interfaces\n";
    }
#endif
}
