#pragma once
#include "scene.h"
#include <algorithm>
#include <cmath>
#include <vector>

// Responsive, inertialess flight in the shared left-handed world.
class Ship {
public:
    static constexpr float speed = 6.0f, boost = 2.0f, turnRate = 1.4f;
    static constexpr float pitchLimit = DirectX::XM_PIDIV2 - 0.05f;
    void reset() { position_ = {0, 0, 0}; yaw_ = pitch_ = 0; }
    DirectX::XMVECTOR forward() const {
        return DirectX::XMVectorSet(std::sin(yaw_)*std::cos(pitch_), std::sin(pitch_),
            std::cos(yaw_)*std::cos(pitch_), 0);
    }
    void update(float thrust, float yaw, float pitch, float vertical, bool fast, float dt) {
        using namespace DirectX;
        if (dt <= 0) return;
        yaw_ = std::remainder(yaw_ + yaw * turnRate * dt, XM_2PI);
        pitch_ = std::clamp(pitch_ + pitch * turnRate * dt, -pitchLimit, pitchLimit);
        XMVECTOR delta = XMVectorAdd(XMVectorScale(forward(), thrust), XMVectorSet(0, vertical, 0, 0));
        const float length = XMVectorGetX(XMVector3Length(delta));
        if (length > 1) delta = XMVectorScale(delta, 1 / length);
        XMStoreFloat3(&position_, XMVectorAdd(XMLoadFloat3(&position_),
            XMVectorScale(delta, speed * (fast ? boost : 1.0f) * dt)));
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
    float yaw() const { return yaw_; }
    float pitch() const { return pitch_; }
private:
    DirectX::XMFLOAT3 position_{0,0,0};
    float yaw_ = 0, pitch_ = 0;
};
