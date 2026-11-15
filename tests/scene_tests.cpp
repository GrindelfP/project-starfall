#include "scene.h"
#include <cmath>
#include <iostream>
using namespace DirectX;
int main() {
    auto close = [](float a, float b) { return std::fabs(a-b) < 0.0001f; };
    auto fail = [](const char* message) { std::cerr << message << '\n'; return 1; };
    scene::Object object{{2,3,4},{0,0,XM_PIDIV2},{10,20,30},{1,1,1,1}};
    XMFLOAT3 p{};
    XMStoreFloat3(&p, XMVector3TransformCoord(XMVectorSet(1,0,0,1), scene::world(object)));
    if (!close(p.x,10) || !close(p.y,22) || !close(p.z,30)) return fail("world S*R*T");
    auto proj = scene::projection(1600,900);
    XMStoreFloat3(&p, XMVector3TransformCoord(XMVectorSet(0,0,0.1f,1), proj));
    if (!close(p.z,0)) return fail("near plane");
    XMStoreFloat3(&p, XMVector3TransformCoord(XMVectorSet(0,0,100,1), proj));
    if (!close(p.z,1)) return fail("far plane");
    XMFLOAT4X4 wide{}, square{};
    XMStoreFloat4x4(&wide, proj); XMStoreFloat4x4(&square, scene::projection(900,900));
    if (!close(wide._11 * (1600.0f/900.0f), square._11) || !close(wide._22, square._22)) return fail("aspect");
    for (auto index : scene::cubeIndices) if (index >= scene::cubeVertices.size()) return fail("index bounds");
    return 0;
}
