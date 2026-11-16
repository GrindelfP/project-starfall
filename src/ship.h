#pragma once
#include "scene.h"
#include <algorithm>
#include <cmath>
#include <vector>

// World-space velocity persists without thrust; steering changes thrust direction only.
class Ship {
public:
    static constexpr float acceleration = 3.0f, boost = 2.0f, maxSpeed = 12.0f, turnRate = 1.4f;
    static constexpr float brakeDeceleration = 6.0f; // world units/s²
    static constexpr float mouseSensitivity = 0.0025f; // radians per raw mouse count
    static constexpr float pitchLimit = DirectX::XM_PIDIV2 - 0.05f;
    void reset() { position_ = {0, 0, 0}; velocity_ = {0, 0, 0}; yaw_ = pitch_ = 0; }
    DirectX::XMVECTOR forward() const {
        return DirectX::XMVectorSet(std::sin(yaw_)*std::cos(pitch_), std::sin(pitch_),
            std::cos(yaw_)*std::cos(pitch_), 0);
    }
    void look(float dx, float dy) {
        yaw_ = std::remainder(yaw_ + dx * mouseSensitivity, DirectX::XM_2PI);
        pitch_ = std::clamp(pitch_ - dy * mouseSensitivity, -pitchLimit, pitchLimit);
    }
    void update(float thrust, float lateral, float yaw, float pitch, float vertical, bool fast, bool brake, float dt) {
        using namespace DirectX;
        if (dt <= 0) return;
        yaw_ = std::remainder(yaw_ + yaw * turnRate * dt, XM_2PI);
        pitch_ = std::clamp(pitch_ + pitch * turnRate * dt, -pitchLimit, pitchLimit);
        const XMVECTOR right = XMVectorSet(std::cos(yaw_), 0, -std::sin(yaw_), 0);
        XMVECTOR delta = XMVectorAdd(XMVectorAdd(XMVectorScale(forward(), thrust), XMVectorScale(right, lateral)), XMVectorSet(0, vertical, 0, 0));
        const float length = XMVectorGetX(XMVector3Length(delta));
        if (length > 1) delta = XMVectorScale(delta, 1 / length);
        const XMVECTOR oldVelocity = XMLoadFloat3(&velocity_);
        XMVECTOR newVelocity = XMVectorAdd(oldVelocity,
            XMVectorScale(delta, acceleration * (fast ? boost : 1.0f) * dt));
        float speed = XMVectorGetX(XMVector3Length(newVelocity));
        if (brake && speed > 0) {
            newVelocity = XMVectorScale(newVelocity, std::max(0.0f, speed - brakeDeceleration * dt) / speed);
            speed = XMVectorGetX(XMVector3Length(newVelocity));
        }
        if (speed > maxSpeed) newVelocity = XMVectorScale(newVelocity, maxSpeed / speed);
        XMStoreFloat3(&position_, XMVectorAdd(XMLoadFloat3(&position_),
            XMVectorScale(XMVectorAdd(oldVelocity, newVelocity), 0.5f * dt)));
        XMStoreFloat3(&velocity_, newVelocity);
    }
    DirectX::XMMATRIX chaseView() const {
        using namespace DirectX;
        const XMVECTOR target = XMLoadFloat3(&position_);
        // Offset follows heading but remains world-up, keeping the horizon steady.
        const XMVECTOR eye = XMVectorAdd(XMVectorSubtract(target, XMVectorScale(forward(), 7.0f)), XMVectorSet(0, 2.5f, 0, 0));
        return XMMatrixLookAtLH(eye, target, XMVectorSet(0,1,0,0));
    }
    std::vector<scene::Object> objects() const {
        using namespace DirectX;
        const XMMATRIX rotation = XMMatrixRotationRollPitchYaw(-pitch_, yaw_, 0);
        std::vector<scene::Object> result;
        auto part = [&](XMFLOAT3 scale, XMFLOAT3 offset, XMFLOAT4 color) {
            XMFLOAT3 worldOffset{};
            XMStoreFloat3(&worldOffset, XMVector3TransformNormal(XMLoadFloat3(&offset), rotation));
            result.push_back({scale, {-pitch_, yaw_, 0},
                {position_.x+worldOffset.x, position_.y+worldOffset.y, position_.z+worldOffset.z}, color});
        };
        part({0.8f,0.45f,2.5f},{0,0,0},{0.8f,0.9f,0.95f,1});
        part({0.55f,0.3f,0.75f},{0,0,1.45f},{0.15f,0.8f,0.95f,1});
        part({0.55f,0.35f,0.8f},{0,0.36f,0.35f},{0.08f,0.35f,0.55f,1});
        part({1.35f,0.16f,1.2f},{-0.95f,-0.05f,-0.15f},{0.2f,0.75f,0.9f,1});
        part({1.35f,0.16f,1.2f},{0.95f,-0.05f,-0.15f},{0.2f,0.75f,0.9f,1});
        part({0.42f,0.35f,0.65f},{-0.53f,0,-1.0f},{1.0f,0.35f,0.04f,1});
        part({0.42f,0.35f,0.65f},{0.53f,0,-1.0f},{1.0f,0.35f,0.04f,1});
        return result;
    }
    DirectX::XMFLOAT3 position() const { return position_; }
    DirectX::XMFLOAT3 velocity() const { return velocity_; }
    float yaw() const { return yaw_; }
    float pitch() const { return pitch_; }
private:
    DirectX::XMFLOAT3 position_{0,0,0};
    DirectX::XMFLOAT3 velocity_{0,0,0};
    float yaw_ = 0, pitch_ = 0;
};
