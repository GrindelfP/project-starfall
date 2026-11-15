#pragma once
#include <DirectXMath.h>
#include <array>
#include <cstdint>

// Shared, renderer-independent scene data. Matrices use DirectXMath row vectors.
namespace scene {
struct Vertex { DirectX::XMFLOAT3 position; };
struct Object {
    DirectX::XMFLOAT3 scale, rotation, translation;
    DirectX::XMFLOAT4 color;
};
inline constexpr std::array<Vertex, 8> cubeVertices{{
    {{-0.5f,-0.5f,-0.5f}}, {{-0.5f,0.5f,-0.5f}}, {{0.5f,0.5f,-0.5f}}, {{0.5f,-0.5f,-0.5f}},
    {{-0.5f,-0.5f,0.5f}}, {{-0.5f,0.5f,0.5f}}, {{0.5f,0.5f,0.5f}}, {{0.5f,-0.5f,0.5f}}
}};
// Outward faces, clockwise on screen from outside in the LH coordinate system.
inline constexpr std::array<std::uint16_t, 36> cubeIndices{{
    0,1,2, 0,2,3, 7,6,5, 7,5,4,
    4,5,1, 4,1,0, 3,2,6, 3,6,7,
    1,5,6, 1,6,2, 4,0,3, 4,3,7
}};
inline constexpr std::array<Object, 3> referenceObjects{{
    {{1.6f,1.6f,1.6f}, {0.25f,0.5f,0.0f}, {-0.45f,0.0f,5.0f}, {0.95f,0.35f,0.12f,1.0f}},
    {{1.8f,1.8f,1.8f}, {0.0f,-0.3f,0.0f}, {0.65f,0.1f,7.0f}, {0.12f,0.65f,0.95f,1.0f}},
    {{0.65f,1.8f,0.65f}, {0.2f,0.3f,0.5f}, {2.0f,-0.25f,6.0f}, {0.55f,0.9f,0.25f,1.0f}}
}};
inline DirectX::XMMATRIX world(const Object& o) {
    using namespace DirectX;
    return XMMatrixScaling(o.scale.x,o.scale.y,o.scale.z) *
        XMMatrixRotationRollPitchYaw(o.rotation.x,o.rotation.y,o.rotation.z) *
        XMMatrixTranslation(o.translation.x,o.translation.y,o.translation.z);
}
inline DirectX::XMMATRIX view() { return DirectX::XMMatrixIdentity(); }
inline DirectX::XMMATRIX projection(unsigned width, unsigned height) {
    return DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4,
        static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);
}
inline DirectX::XMMATRIX viewProjection(unsigned width, unsigned height) { return view() * projection(width,height); }
}
