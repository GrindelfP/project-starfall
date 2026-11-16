#pragma once
#include "scene.h"
#include <algorithm>
#include <cmath>

// Continuous separating-axis test for translating oriented boxes. The moving box
// retains its final orientation during the sweep; touching counts as contact.
namespace collision {
inline bool sweptBoxes(const scene::Object& moving, DirectX::XMFLOAT3 displacement,
                       const scene::Object& fixed) {
    using namespace DirectX;
    const XMMATRIX ma = XMMatrixRotationRollPitchYaw(moving.rotation.x, moving.rotation.y, moving.rotation.z);
    const XMMATRIX mb = XMMatrixRotationRollPitchYaw(fixed.rotation.x, fixed.rotation.y, fixed.rotation.z);
    XMVECTOR a[3], b[3];
    for (int i=0; i<3; ++i) { a[i]=ma.r[i]; b[i]=mb.r[i]; }
    const float ah[3]={moving.scale.x/2,moving.scale.y/2,moving.scale.z/2};
    const float bh[3]={fixed.scale.x/2,fixed.scale.y/2,fixed.scale.z/2};
    const XMVECTOR delta=XMVectorSubtract(XMLoadFloat3(&moving.translation), XMLoadFloat3(&fixed.translation));
    const XMVECTOR velocity=XMLoadFloat3(&displacement);
    float enter=0, exit=1;
    auto axisTest = [&](XMVECTOR axis) {
        const float len2=XMVectorGetX(XMVector3LengthSq(axis));
        if (len2 < 1e-12f) return true;
        axis=XMVectorScale(axis, 1/std::sqrt(len2));
        float radius=0;
        for (int i=0; i<3; ++i) {
            radius += ah[i]*std::fabs(XMVectorGetX(XMVector3Dot(a[i],axis)));
            radius += bh[i]*std::fabs(XMVectorGetX(XMVector3Dot(b[i],axis)));
        }
        // Small world-space tolerance avoids numerical misses at touching faces.
        radius += 1e-5f;
        const float p=XMVectorGetX(XMVector3Dot(delta,axis));
        const float v=XMVectorGetX(XMVector3Dot(velocity,axis));
        if (std::fabs(v)<1e-8f) return std::fabs(p)<=radius;
        float t0=(-radius-p)/v, t1=(radius-p)/v;
        if (t0>t1) std::swap(t0,t1);
        enter=std::max(enter,t0); exit=std::min(exit,t1);
        return enter<=exit;
    };
    for (int i=0;i<3;++i) if (!axisTest(a[i]) || !axisTest(b[i])) return false;
    for (int i=0;i<3;++i) for (int j=0;j<3;++j)
        if (!axisTest(XMVector3Cross(a[i],b[j]))) return false;
    return true;
}
}
