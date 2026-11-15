#include "dx12.h"
#include <d3d12sdklayers.h>
#include <dxgidebug.h>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <vector>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <utility>
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
    createScene();
}
void Dx12::createTargets(unsigned width, unsigned height) {
    auto handle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < 2; ++i) {
        check(swap_->GetBuffer(i, IID_PPV_ARGS(&targets_[i])), "GetBuffer");
        device_->CreateRenderTargetView(targets_[i].Get(), nullptr, handle); handle.ptr += rtvStride_;
    }
    if (!dsvHeap_) {
        D3D12_DESCRIPTOR_HEAP_DESC heap{}; heap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV; heap.NumDescriptors = 1;
        check(device_->CreateDescriptorHeap(&heap, IID_PPV_ARGS(&dsvHeap_)), "Create DSV heap");
    }
    D3D12_HEAP_PROPERTIES properties{}; properties.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC desc{}; desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = width; desc.Height = height; desc.DepthOrArraySize = 1; desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; desc.SampleDesc.Count = 1;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    D3D12_CLEAR_VALUE clear{}; clear.Format = desc.Format; clear.DepthStencil.Depth = 1;
    check(device_->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE, &desc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear, IID_PPV_ARGS(&depth_)), "Create depth buffer");
    device_->CreateDepthStencilView(depth_.Get(), nullptr, dsvHeap_->GetCPUDescriptorHandleForHeapStart());
    viewport_ = {0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1};
    scissor_ = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    DirectX::XMStoreFloat4x4(&projection_, scene::projection(width, height));
    log_ << "Client/viewport: " << width << " x " << height << '\n';
}
void Dx12::createScene() {
    D3D12_ROOT_PARAMETER parameter{}; parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    parameter.Constants.ShaderRegister = 0; parameter.Constants.Num32BitValues = 20;
    parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    D3D12_ROOT_SIGNATURE_DESC signature{}; signature.NumParameters = 1; signature.pParameters = &parameter;
    signature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    ComPtr<ID3DBlob> blob, errors;
    HRESULT result = D3D12SerializeRootSignature(&signature, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &errors);
    if (FAILED(result) && errors) log_ << "Root signature: " << static_cast<const char*>(errors->GetBufferPointer()) << '\n';
    check(result, "D3D12SerializeRootSignature");
    check(device_->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&root_)), "CreateRootSignature");
    wchar_t executable[32768]{};
    const auto len = GetModuleFileNameW(nullptr, executable, static_cast<DWORD>(std::size(executable)));
    if (!len || len == std::size(executable)) throw std::runtime_error("Cannot resolve executable path for DXIL");
    auto read = [&](const wchar_t* name) {
        const auto path = std::filesystem::path(executable).parent_path() / L"shaders" / name;
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) throw std::runtime_error("Cannot open DXIL shader: " + path.string());
        const auto size = file.tellg(); if (size <= 0) throw std::runtime_error("Empty DXIL shader: " + path.string());
        std::vector<char> data(static_cast<size_t>(size)); file.seekg(0); file.read(data.data(), size);
        if (!file) throw std::runtime_error("Cannot read DXIL shader: " + path.string()); return data;
    };
    const auto vs = read(L"scene12_vs.dxil"), ps = read(L"scene12_ps.dxil");
    constexpr D3D12_INPUT_ELEMENT_DESC input[] = {{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};
    D3D12_GRAPHICS_PIPELINE_STATE_DESC p{}; p.pRootSignature = root_.Get();
    p.VS = {vs.data(), vs.size()}; p.PS = {ps.data(), ps.size()}; p.InputLayout = {input, 1};
    p.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE; p.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
    p.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD; p.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    p.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO; p.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    p.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP; p.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    p.SampleMask = UINT_MAX; p.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; p.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    p.RasterizerState.DepthClipEnable = TRUE; p.DepthStencilState.DepthEnable = TRUE;
    p.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; p.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    p.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK; p.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    p.DepthStencilState.FrontFace = {D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_COMPARISON_FUNC_ALWAYS};
    p.DepthStencilState.BackFace = p.DepthStencilState.FrontFace;
    p.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; p.NumRenderTargets = 1;
    p.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; p.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT; p.SampleDesc.Count = 1;
    check(device_->CreateGraphicsPipelineState(&p, IID_PPV_ARGS(&pipeline_)), "CreateGraphicsPipelineState");
    auto upload = [&](const void* data, size_t size, ComPtr<ID3D12Resource>& resource) {
        D3D12_HEAP_PROPERTIES properties{}; properties.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC desc{}; desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; desc.Width = size; desc.Height = 1;
        desc.DepthOrArraySize = 1; desc.MipLevels = 1; desc.SampleDesc.Count = 1; desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        check(device_->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&resource)), "Create upload buffer");
        void* mapped = nullptr; D3D12_RANGE noRead{0, 0}; check(resource->Map(0, &noRead, &mapped), "Map upload buffer");
        std::memcpy(mapped, data, size); resource->Unmap(0, nullptr);
    };
    upload(scene::cubeVertices.data(), sizeof(scene::cubeVertices), vertices_);
    upload(scene::cubeIndices.data(), sizeof(scene::cubeIndices), indices_);
    log_ << "Loaded DXC SM6 shaders; indexed reference scene ready\n";
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
void Dx12::enableTiming() {
    D3D12_QUERY_HEAP_DESC q{}; q.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP; q.Count = 2;
    check(device_->CreateQueryHeap(&q, IID_PPV_ARGS(&timestamps_)), "Create timestamp heap");
    D3D12_HEAP_PROPERTIES heap{}; heap.Type = D3D12_HEAP_TYPE_READBACK;
    D3D12_RESOURCE_DESC buffer{}; buffer.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; buffer.Width = 2 * sizeof(UINT64);
    buffer.Height = 1; buffer.DepthOrArraySize = 1; buffer.MipLevels = 1; buffer.SampleDesc.Count = 1; buffer.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    check(device_->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &buffer, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&timestampReadback_)), "Create timestamp readback");
    check(queue_->GetTimestampFrequency(&timestampFrequency_), "GetTimestampFrequency");
}
std::vector<GpuSample> Dx12::takeTimings() { return std::exchange(samples_, {}); }
void Dx12::present(std::span<const scene::Object> objects, DirectX::FXMMATRIX view, bool clearOnly) {
    check(allocator_->Reset(), "Reset allocator");
    check(commands_->Reset(allocator_.Get(), pipeline_.Get()), "Reset command list");
    const UINT frame = swap_->GetCurrentBackBufferIndex();
    D3D12_RESOURCE_BARRIER barrier{}; barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = targets_[frame].Get(); barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT; barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    commands_->ResourceBarrier(1, &barrier);
    if (timestamps_) commands_->EndQuery(timestamps_.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0);
    auto rtv = rtvHeap_->GetCPUDescriptorHandleForHeapStart(); rtv.ptr += frame * rtvStride_;
    constexpr float clear[] = {0.025f, 0.12f, 0.25f, 1.0f}; commands_->ClearRenderTargetView(rtv, clear, 0, nullptr);
    if (!clearOnly) {
        auto dsv = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
        commands_->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1, 0, 0, nullptr);
        commands_->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
        commands_->RSSetViewports(1, &viewport_); commands_->RSSetScissorRects(1, &scissor_);
        commands_->SetGraphicsRootSignature(root_.Get());
        commands_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        D3D12_VERTEX_BUFFER_VIEW vb{vertices_->GetGPUVirtualAddress(), sizeof(scene::cubeVertices), sizeof(scene::Vertex)};
        D3D12_INDEX_BUFFER_VIEW ib{indices_->GetGPUVirtualAddress(), sizeof(scene::cubeIndices), DXGI_FORMAT_R16_UINT};
        commands_->IASetVertexBuffers(0, 1, &vb); commands_->IASetIndexBuffer(&ib);
        struct Constants { DirectX::XMFLOAT4X4 wvp; DirectX::XMFLOAT4 color; };
        for (const auto& object : objects) {
            Constants values{}; DirectX::XMStoreFloat4x4(&values.wvp, scene::world(object) * view * DirectX::XMLoadFloat4x4(&projection_));
            values.color = object.color; commands_->SetGraphicsRoot32BitConstants(0, 20, &values, 0);
            commands_->DrawIndexedInstanced(static_cast<UINT>(scene::cubeIndices.size()), 1, 0, 0, 0);
        }
    }
    if (timestamps_) { commands_->EndQuery(timestamps_.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 1); commands_->ResolveQueryData(timestamps_.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0, 2, timestampReadback_.Get(), 0); }
    ComPtr<ID3D12Resource> readback;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{}; UINT64 bytes = 0;
    if (!capturePath_.empty()) {
        auto desc = targets_[frame]->GetDesc(); UINT rows = 0; UINT64 rowBytes = 0;
        device_->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, &rows, &rowBytes, &bytes);
        D3D12_HEAP_PROPERTIES heap{}; heap.Type = D3D12_HEAP_TYPE_READBACK;
        D3D12_RESOURCE_DESC buffer{}; buffer.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; buffer.Width = bytes;
        buffer.Height = 1; buffer.DepthOrArraySize = 1; buffer.MipLevels = 1; buffer.SampleDesc.Count = 1; buffer.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        check(device_->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &buffer, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
            IID_PPV_ARGS(&readback)), "Create capture buffer");
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET; barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
        commands_->ResourceBarrier(1, &barrier);
        D3D12_TEXTURE_COPY_LOCATION source{}; source.pResource = targets_[frame].Get(); source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        D3D12_TEXTURE_COPY_LOCATION dest{}; dest.pResource = readback.Get(); dest.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT; dest.PlacedFootprint = footprint;
        commands_->CopyTextureRegion(&dest, 0, 0, 0, &source, nullptr);
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    } else barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT; commands_->ResourceBarrier(1, &barrier);
    check(commands_->Close(), "Close command list"); ID3D12CommandList* lists[] = {commands_.Get()}; queue_->ExecuteCommandLists(1, lists);
    check(swap_->Present(vsync_ ? 1 : 0, 0), "Present"); wait();
    if (timestamps_) {
        UINT64* values = nullptr; D3D12_RANGE range{0, 2 * sizeof(UINT64)};
        check(timestampReadback_->Map(0, &range, reinterpret_cast<void**>(&values)), "Map timestamps");
        const bool valid = timestampFrequency_ && values[1] >= values[0];
        samples_.push_back({timingFrame_++, valid ? 1000.0 * (values[1] - values[0]) / timestampFrequency_ : 0.0, valid});
        D3D12_RANGE noWrite{0, 0}; timestampReadback_->Unmap(0, &noWrite);
    }
    if (readback) {
        const UINT width = footprint.Footprint.Width, height = footprint.Footprint.Height;
        BITMAPFILEHEADER file{}; BITMAPINFOHEADER info{};
        file.bfType = 0x4d42; file.bfOffBits = sizeof(file) + sizeof(info); file.bfSize = file.bfOffBits + width * height * 4;
        info.biSize = sizeof(info); info.biWidth = static_cast<LONG>(width); info.biHeight = -static_cast<LONG>(height);
        info.biPlanes = 1; info.biBitCount = 32; info.biCompression = BI_RGB; info.biSizeImage = width * height * 4;
        void* mapped = nullptr; D3D12_RANGE range{0, static_cast<SIZE_T>(bytes)}; check(readback->Map(0, &range, &mapped), "Map capture buffer");
        std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 4);
        for (UINT y = 0; y < height; ++y) {
            const auto* row = static_cast<const unsigned char*>(mapped) + footprint.Offset + static_cast<size_t>(y) * footprint.Footprint.RowPitch;
            for (UINT x = 0; x < width; ++x) {
                auto* pixel = pixels.data() + (static_cast<size_t>(y) * width + x) * 4;
                pixel[0] = row[x * 4 + 2]; pixel[1] = row[x * 4 + 1]; pixel[2] = row[x * 4]; pixel[3] = 255;
            }
        }
        D3D12_RANGE noWrite{0, 0}; readback->Unmap(0, &noWrite);
        if (!capturePath_.parent_path().empty()) std::filesystem::create_directories(capturePath_.parent_path());
        std::ofstream output(capturePath_, std::ios::binary); output.write(reinterpret_cast<const char*>(&file), sizeof(file));
        output.write(reinterpret_cast<const char*>(&info), sizeof(info)); output.write(reinterpret_cast<const char*>(pixels.data()), static_cast<std::streamsize>(pixels.size()));
        if (!output) throw std::runtime_error("Cannot write frame capture: " + capturePath_.string());
        log_ << "Frame capture: " << capturePath_.string() << '\n'; capturePath_.clear();
    }
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
    timestamps_.Reset(); timestampReadback_.Reset();
    pipeline_.Reset(); root_.Reset(); commands_.Reset(); allocator_.Reset(); rtvHeap_.Reset(); dsvHeap_.Reset(); swap_.Reset(); fence_.Reset(); queue_.Reset();
#ifdef _DEBUG
    if (device_) {
        dumpMessages(); ComPtr<ID3D12DebugDevice> debug;
        if (SUCCEEDED(device_.As(&debug))) { log_ << "Live objects (device and diagnostics remain):\n"; debug->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL); dumpMessages(); }
    }
#endif
    device_.Reset(); if (event_) CloseHandle(event_);
}



