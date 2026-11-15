#pragma once
#include <DirectXMath.h>
#include <algorithm>
#include <cmath>

// Left-handed coordinates; the reference pose looks along +Z with +Y up.
class Camera {
public:
    static constexpr float speed = 3.0f;
    static constexpr float fastMultiplier = 3.0f;
    static constexpr float mouseSensitivity = 0.0025f;
    static constexpr float pitchLimit = DirectX::XM_PIDIV2 - 0.01f;

    void reset() { position_ = {0, 0, 0}; yaw_ = pitch_ = 0; }
    void look(float dx, float dy) {
        yaw_ = std::remainder(yaw_ + dx * mouseSensitivity, DirectX::XM_2PI);
        pitch_ = std::clamp(pitch_ - dy * mouseSensitivity, -pitchLimit, pitchLimit);
    }
    DirectX::XMVECTOR forward() const {
        return DirectX::XMVectorSet(std::sin(yaw_) * std::cos(pitch_), std::sin(pitch_),
                                   std::cos(yaw_) * std::cos(pitch_), 0);
    }
    DirectX::XMVECTOR right() const {
        return DirectX::XMVectorSet(std::cos(yaw_), 0, -std::sin(yaw_), 0);
    }
    DirectX::XMVECTOR up() const { return DirectX::XMVector3Cross(forward(), right()); }
    void move(float sideways, float vertical, float ahead, float dt, bool fast) {
        using namespace DirectX;
        XMVECTOR direction = XMVectorAdd(XMVectorAdd(XMVectorScale(right(), sideways),
            XMVectorSet(0, vertical, 0, 0)), XMVectorScale(forward(), ahead));
        const float length = XMVectorGetX(XMVector3Length(direction));
        if (length > 0 && dt > 0) {
            direction = XMVectorScale(direction, speed * (fast ? fastMultiplier : 1.0f) * dt / length);
            XMStoreFloat3(&position_, XMVectorAdd(XMLoadFloat3(&position_), direction));
        }
    }
    DirectX::XMMATRIX view() const {
        using namespace DirectX;
        const XMVECTOR position = XMLoadFloat3(&position_);
        return XMMatrixLookToLH(position, forward(), up());
    }
    DirectX::XMFLOAT3 position() const { return position_; }
    float yaw() const { return yaw_; }
    float pitch() const { return pitch_; }
private:
    DirectX::XMFLOAT3 position_{0, 0, 0};
    float yaw_ = 0, pitch_ = 0;
};
