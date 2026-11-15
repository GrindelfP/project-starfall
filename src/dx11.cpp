#include "dx11.h"
#include <d3d11sdklayers.h>
#include <d3dcompiler.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>

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
    createScene();
}

void Dx11::createScene() {
    wchar_t executable[32768]{};
    const DWORD length = GetModuleFileNameW(nullptr, executable, static_cast<DWORD>(std::size(executable)));
    if (!length || length == std::size(executable)) throw std::runtime_error("Cannot resolve executable path for shaders");
    const auto path = std::filesystem::path(executable).parent_path() / L"shaders" / L"scene.hlsl";
    const auto vs = compileShader(path, "VSMain", "vs_5_0");
    const auto ps = compileShader(path, "PSMain", "ps_5_0");
    checkDevice(device_->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &vertexShader_), "CreateVertexShader");
    checkDevice(device_->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &pixelShader_), "CreatePixelShader");
    constexpr D3D11_INPUT_ELEMENT_DESC elements[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    checkDevice(device_->CreateInputLayout(elements, static_cast<UINT>(std::size(elements)), vs->GetBufferPointer(), vs->GetBufferSize(), &inputLayout_), "CreateInputLayout");
    D3D11_BUFFER_DESC buffer{};
    buffer.ByteWidth = sizeof(scene::cubeVertices); buffer.Usage = D3D11_USAGE_IMMUTABLE; buffer.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA data{}; data.pSysMem = scene::cubeVertices.data();
    checkDevice(device_->CreateBuffer(&buffer, &data, &vertexBuffer_), "Create vertex buffer");
    buffer.ByteWidth = sizeof(scene::cubeIndices); buffer.BindFlags = D3D11_BIND_INDEX_BUFFER; data.pSysMem = scene::cubeIndices.data();
    checkDevice(device_->CreateBuffer(&buffer, &data, &indexBuffer_), "Create index buffer");
    buffer.ByteWidth = sizeof(DirectX::XMFLOAT4X4) + sizeof(DirectX::XMFLOAT4); buffer.Usage = D3D11_USAGE_DEFAULT; buffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    checkDevice(device_->CreateBuffer(&buffer, nullptr, &constants_), "Create constant buffer");
    D3D11_RASTERIZER_DESC raster{};
    raster.FillMode = D3D11_FILL_SOLID; raster.CullMode = D3D11_CULL_BACK; raster.DepthClipEnable = TRUE;
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
    depth.DepthEnable = TRUE; depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depth.DepthFunc = D3D11_COMPARISON_LESS; depth.StencilEnable = FALSE;
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
    D3D11_TEXTURE2D_DESC depthDesc{};
    depthDesc.Width = width; depthDesc.Height = height; depthDesc.MipLevels = 1; depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; depthDesc.SampleDesc.Count = 1;
    depthDesc.Usage = D3D11_USAGE_DEFAULT; depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    ComPtr<ID3D11Texture2D> depthTexture;
    checkDevice(device_->CreateTexture2D(&depthDesc, nullptr, &depthTexture), "Create depth texture");
    checkDevice(device_->CreateDepthStencilView(depthTexture.Get(), nullptr, &depthTarget_), "Create depth view");
    DirectX::XMStoreFloat4x4(&projection_, scene::projection(width, height));
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
    depthTarget_.Reset();
    checkDevice(swapChain_->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0), "ResizeBuffers");
    createTarget(width, height);
}

void Dx11::enableTiming() {
    D3D11_QUERY_DESC desc{};
    for (auto& slot : queries_) {
        desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT; checkDevice(device_->CreateQuery(&desc, &slot.disjoint), "Create disjoint query");
        desc.Query = D3D11_QUERY_TIMESTAMP; checkDevice(device_->CreateQuery(&desc, &slot.start), "Create start query");
        checkDevice(device_->CreateQuery(&desc, &slot.end), "Create end query");
    }
    timing_ = true;
}
void Dx11::pollTimings() {
    for (auto& slot : queries_) if (slot.pending) {
        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT d{}; UINT64 start = 0, end = 0;
        const auto a = context_->GetData(slot.disjoint.Get(), &d, sizeof(d), D3D11_ASYNC_GETDATA_DONOTFLUSH);
        const auto b = context_->GetData(slot.start.Get(), &start, sizeof(start), D3D11_ASYNC_GETDATA_DONOTFLUSH);
        const auto c = context_->GetData(slot.end.Get(), &end, sizeof(end), D3D11_ASYNC_GETDATA_DONOTFLUSH);
        checkDevice(a, "Get disjoint query"); checkDevice(b, "Get start query"); checkDevice(c, "Get end query");
        if (a == S_OK && b == S_OK && c == S_OK) {
            const bool valid = !d.Disjoint && d.Frequency && end >= start;
            samples_.push_back({slot.frame, valid ? 1000.0 * (end - start) / d.Frequency : 0.0, valid});
            slot.pending = false;
        }
    }
}
std::vector<GpuSample> Dx11::takeTimings() { if (timing_) pollTimings(); return std::exchange(samples_, {}); }

void Dx11::present(std::span<const scene::Object> objects, DirectX::FXMMATRIX view, bool verifyOrder) {
    QuerySlot* query = nullptr;
    if (timing_) {
        pollTimings(); auto& slot = queries_[timingFrame_ % queries_.size()];
        if (!slot.pending) { query = &slot; slot.frame = timingFrame_; context_->Begin(slot.disjoint.Get()); context_->End(slot.start.Get()); }
        else samples_.push_back({timingFrame_, 0.0, false});
        ++timingFrame_;
    }
    constexpr float clear[] = { 0.025f, 0.12f, 0.25f, 1.0f };
    context_->ClearRenderTargetView(target_.Get(), clear);
    context_->ClearDepthStencilView(depthTarget_.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    ID3D11RenderTargetView* target = target_.Get();
    context_->OMSetRenderTargets(1, &target, depthTarget_.Get());
    context_->OMSetBlendState(blend_.Get(), nullptr, 0xffffffff);
    context_->OMSetDepthStencilState(depth_.Get(), 0);
    context_->RSSetState(rasterizer_.Get());
    context_->IASetInputLayout(inputLayout_.Get());
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11Buffer* buffer = vertexBuffer_.Get();
    constexpr UINT stride = sizeof(scene::Vertex), offset = 0;
    context_->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
    context_->IASetIndexBuffer(indexBuffer_.Get(), DXGI_FORMAT_R16_UINT, 0);
    ID3D11Buffer* constants = constants_.Get();
    context_->VSSetConstantBuffers(0, 1, &constants);
    context_->VSSetShader(vertexShader_.Get(), nullptr, 0);
    context_->PSSetShader(pixelShader_.Get(), nullptr, 0);
    struct Constants { DirectX::XMFLOAT4X4 wvp; DirectX::XMFLOAT4 color; };
    static_assert(sizeof(Constants) == 80);
    auto draw = [&](const scene::Object& object) {
        Constants values{};
        DirectX::XMStoreFloat4x4(&values.wvp, scene::world(object) * view * DirectX::XMLoadFloat4x4(&projection_));
        values.color = object.color;
        context_->UpdateSubresource(constants_.Get(), 0, nullptr, &values, 0, 0);
        context_->DrawIndexed(static_cast<UINT>(scene::cubeIndices.size()), 0, 0);
    };
    for (const auto& object : objects) draw(object);
    if (query) { context_->End(query->end.Get()); context_->End(query->disjoint.Get()); query->pending = true; }
    if (verifyOrder) {
        ComPtr<ID3D11Texture2D> backBuffer, staging;
        checkDevice(swapChain_->GetBuffer(0, IID_PPV_ARGS(&backBuffer)), "GetBuffer for depth check");
        D3D11_TEXTURE2D_DESC desc{}; backBuffer->GetDesc(&desc);
        desc.Usage = D3D11_USAGE_STAGING; desc.BindFlags = 0; desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ; desc.MiscFlags = 0;
        checkDevice(device_->CreateTexture2D(&desc, nullptr, &staging), "Create depth check staging texture");
        auto pixels = [&]() {
            context_->CopyResource(staging.Get(), backBuffer.Get());
            D3D11_MAPPED_SUBRESOURCE mapped{};
            checkDevice(context_->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped), "Map depth check texture");
            std::vector<unsigned char> result(static_cast<size_t>(desc.Width) * desc.Height * 4);
            for (UINT y = 0; y < desc.Height; ++y)
                std::memcpy(result.data() + static_cast<size_t>(y) * desc.Width * 4,
                    static_cast<const unsigned char*>(mapped.pData) + static_cast<size_t>(y) * mapped.RowPitch, desc.Width * 4);
            context_->Unmap(staging.Get(), 0);
            return result;
        };
        const auto normal = pixels();
        context_->ClearRenderTargetView(target_.Get(), clear);
        context_->ClearDepthStencilView(depthTarget_.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        for (auto it = objects.rbegin(); it != objects.rend(); ++it) draw(*it);
        if (pixels() != normal) throw std::runtime_error("Depth check: reversed submission changed rendered pixels");
        log_ << "Depth check: normal and reversed submission produced identical pixels\n";
    }
    if (!capturePath_.empty()) saveCapture();
    checkDevice(swapChain_->Present(vsync_ ? 1 : 0, 0), "Present");
}

void Dx11::saveCapture() {
    ComPtr<ID3D11Texture2D> source, staging;
    checkDevice(swapChain_->GetBuffer(0, IID_PPV_ARGS(&source)), "Get capture buffer");
    D3D11_TEXTURE2D_DESC desc{};
    source->GetDesc(&desc);
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;
    checkDevice(device_->CreateTexture2D(&desc, nullptr, &staging), "Create capture staging texture");
    context_->CopyResource(staging.Get(), source.Get());
    D3D11_MAPPED_SUBRESOURCE mapped{};
    checkDevice(context_->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped), "Map capture texture");
    std::vector<unsigned char> pixels;
    try {
        pixels.resize(static_cast<size_t>(desc.Width) * desc.Height * 4);
        for (UINT y = 0; y < desc.Height; ++y) {
            const auto* row = static_cast<const unsigned char*>(mapped.pData) + static_cast<size_t>(y) * mapped.RowPitch;
            auto* destination = pixels.data() + static_cast<size_t>(y) * desc.Width * 4;
            for (UINT x = 0; x < desc.Width; ++x) {
                destination[x * 4] = row[x * 4 + 2];
                destination[x * 4 + 1] = row[x * 4 + 1];
                destination[x * 4 + 2] = row[x * 4];
                destination[x * 4 + 3] = 255;
            }
        }
    } catch (...) { context_->Unmap(staging.Get(), 0); throw; }
    context_->Unmap(staging.Get(), 0);
    BITMAPFILEHEADER file{};
    BITMAPINFOHEADER info{};
    file.bfType = 0x4d42;
    file.bfOffBits = sizeof(file) + sizeof(info);
    file.bfSize = file.bfOffBits + static_cast<DWORD>(pixels.size());
    info.biSize = sizeof(info);
    info.biWidth = static_cast<LONG>(desc.Width);
    info.biHeight = -static_cast<LONG>(desc.Height); // top-down rows
    info.biPlanes = 1;
    info.biBitCount = 32;
    info.biCompression = BI_RGB;
    if (!capturePath_.parent_path().empty()) std::filesystem::create_directories(capturePath_.parent_path());
    std::ofstream output(capturePath_, std::ios::binary);
    output.write(reinterpret_cast<const char*>(&file), sizeof(file));
    output.write(reinterpret_cast<const char*>(&info), sizeof(info));
    output.write(reinterpret_cast<const char*>(pixels.data()), static_cast<std::streamsize>(pixels.size()));
    output.close();
    if (!output) throw std::runtime_error("Cannot write frame capture: " + capturePath_.string());
    log_ << "Frame capture: " << capturePath_.string() << '\n';
    capturePath_.clear();
}

Dx11::~Dx11() {
    if (context_) { context_->ClearState(); context_->Flush(); }
    for (auto& query : queries_) { query.disjoint.Reset(); query.start.Reset(); query.end.Reset(); }
    target_.Reset();
    depthTarget_.Reset(); indexBuffer_.Reset(); constants_.Reset(); vertexBuffer_.Reset(); inputLayout_.Reset(); vertexShader_.Reset(); pixelShader_.Reset();
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
