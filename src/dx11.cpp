#include "dx11.h"
#include <d3d11sdklayers.h>
#include <d3dcompiler.h>
#include <filesystem>
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
ComPtr<ID3DBlob> compileShader(const std::filesystem::path& path, const char* entry, const char* profile) {
    ComPtr<ID3DBlob> code, errors;
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
    const HRESULT hr = D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entry, profile, flags, 0, &code, &errors);
    if (FAILED(hr)) {
        std::ostringstream message;
        message << "Shader compilation failed: " << path.string() << ", entry " << entry
                << ", target " << profile << " (HRESULT 0x" << std::hex
                << static_cast<unsigned long>(hr) << ")";
        if (errors) message << "\n" << static_cast<const char*>(errors->GetBufferPointer());
        throw std::runtime_error(message.str());
    }
    return code;
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
    createTarget(width, height);
    createTriangle();
}

void Dx11::createTriangle() {
    wchar_t executable[32768]{};
    const DWORD length = GetModuleFileNameW(nullptr, executable, static_cast<DWORD>(std::size(executable)));
    if (!length || length == std::size(executable)) throw std::runtime_error("Cannot resolve executable path for shaders");
    const auto path = std::filesystem::path(executable).parent_path() / L"shaders" / L"triangle.hlsl";
    const auto vs = compileShader(path, "VSMain", "vs_5_0");
    const auto ps = compileShader(path, "PSMain", "ps_5_0");
    checkDevice(device_->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &vertexShader_), "CreateVertexShader");
    checkDevice(device_->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &pixelShader_), "CreatePixelShader");
    constexpr D3D11_INPUT_ELEMENT_DESC elements[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    checkDevice(device_->CreateInputLayout(elements, static_cast<UINT>(std::size(elements)),
        vs->GetBufferPointer(), vs->GetBufferSize(), &inputLayout_), "CreateInputLayout");
    struct Vertex { float x, y, r, g, b; };
    constexpr Vertex vertices[] = {
        { -0.72f, -0.58f, 1.0f, 0.15f, 0.08f },
        {  0.64f, -0.43f, 0.10f, 1.0f, 0.20f },
        { -0.18f,  0.75f, 0.15f, 0.35f, 1.0f }
    };
    D3D11_BUFFER_DESC buffer{};
    buffer.ByteWidth = sizeof(vertices); buffer.Usage = D3D11_USAGE_IMMUTABLE; buffer.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA data{}; data.pSysMem = vertices;
    checkDevice(device_->CreateBuffer(&buffer, &data, &vertexBuffer_), "CreateBuffer");
    D3D11_RASTERIZER_DESC raster{};
    raster.FillMode = D3D11_FILL_SOLID; raster.CullMode = D3D11_CULL_NONE; raster.DepthClipEnable = TRUE;
    checkDevice(device_->CreateRasterizerState(&raster, &rasterizer_), "CreateRasterizerState");
    D3D11_BLEND_DESC blend{};
    auto& targetBlend = blend.RenderTarget[0];
    targetBlend.SrcBlend = D3D11_BLEND_ONE; targetBlend.DestBlend = D3D11_BLEND_ZERO;
    targetBlend.BlendOp = D3D11_BLEND_OP_ADD;
    targetBlend.SrcBlendAlpha = D3D11_BLEND_ONE; targetBlend.DestBlendAlpha = D3D11_BLEND_ZERO;
    targetBlend.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    targetBlend.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    checkDevice(device_->CreateBlendState(&blend, &blend_), "CreateBlendState");
    D3D11_DEPTH_STENCIL_DESC depth{};
    depth.DepthEnable = FALSE; depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depth.DepthFunc = D3D11_COMPARISON_ALWAYS; depth.StencilEnable = FALSE;
    depth.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK; depth.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
    depth.FrontFace = { D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS };
    depth.BackFace = depth.FrontFace;
    checkDevice(device_->CreateDepthStencilState(&depth, &depth_), "CreateDepthStencilState");
    log_ << "Compiled " << path.string() << " (VSMain vs_5_0, PSMain ps_5_0)\n";
}

void Dx11::checkDevice(HRESULT result, const char* operation) {
    if (FAILED(result)) {
        std::ostringstream message;
        message << operation << " failed (HRESULT 0x" << std::hex << std::uppercase
                << static_cast<unsigned long>(result) << ")";
        if (device_) {
            const HRESULT reason = device_->GetDeviceRemovedReason();
            if (FAILED(reason)) message << "; device removal reason 0x" << static_cast<unsigned long>(reason);
        }
        throw std::runtime_error(message.str());
    }
}

void Dx11::createTarget(unsigned width, unsigned height) {
    ComPtr<ID3D11Texture2D> backBuffer;
    checkDevice(swapChain_->GetBuffer(0, IID_PPV_ARGS(&backBuffer)), "GetBuffer");
    checkDevice(device_->CreateRenderTargetView(backBuffer.Get(), nullptr, &target_), "CreateRenderTargetView");
    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MaxDepth = 1.0f;
    context_->RSSetViewports(1, &viewport);
    log_ << "Client/viewport: " << width << " x " << height << '\n';
    log_.flush();
}

void Dx11::resize(unsigned width, unsigned height) {
    if (!width || !height) return;
    context_->OMSetRenderTargets(0, nullptr, nullptr);
    target_.Reset();
    checkDevice(swapChain_->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0), "ResizeBuffers");
    createTarget(width, height);
}

void Dx11::present() {
    constexpr float clear[] = { 0.025f, 0.12f, 0.25f, 1.0f };
    context_->ClearRenderTargetView(target_.Get(), clear);
    ID3D11RenderTargetView* target = target_.Get();
    context_->OMSetRenderTargets(1, &target, nullptr);
    context_->OMSetBlendState(blend_.Get(), nullptr, 0xffffffff);
    context_->OMSetDepthStencilState(depth_.Get(), 0);
    context_->RSSetState(rasterizer_.Get());
    context_->IASetInputLayout(inputLayout_.Get());
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11Buffer* buffer = vertexBuffer_.Get();
    constexpr UINT stride = 5 * sizeof(float), offset = 0;
    context_->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
    context_->VSSetShader(vertexShader_.Get(), nullptr, 0);
    context_->PSSetShader(pixelShader_.Get(), nullptr, 0);
    context_->Draw(3, 0);
    checkDevice(swapChain_->Present(1, 0), "Present");
}

Dx11::~Dx11() {
    if (context_) { context_->ClearState(); context_->Flush(); }
    target_.Reset();
    vertexBuffer_.Reset(); inputLayout_.Reset(); vertexShader_.Reset(); pixelShader_.Reset();
    rasterizer_.Reset(); blend_.Reset(); depth_.Reset();
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
