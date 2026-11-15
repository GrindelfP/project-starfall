#include "camera.h"
#include <cmath>
#include <iostream>
using namespace DirectX;
int main() {
    auto close = [](float a, float b) { return std::fabs(a-b) < 0.0002f; };
    auto fail = [](const char* message) { std::cerr << message << '\n'; return 1; };
    Camera c;
    XMFLOAT4X4 view{}; XMStoreFloat4x4(&view, c.view());
    if (!close(view._11, 1) || !close(view._22, 1) || !close(view._33, 1) || !close(view._44, 1)) return fail("reference view");
    c.look(440, -300);
    auto f = c.forward(), r = c.right(), u = c.up();
    if (!close(XMVectorGetX(XMVector3Length(f)), 1) || !close(XMVectorGetX(XMVector3Length(r)), 1) ||
        !close(XMVectorGetX(XMVector3Length(u)), 1) || !close(XMVectorGetX(XMVector3Dot(f,r)), 0) ||
        !close(XMVectorGetX(XMVector3Dot(f,u)), 0) || !close(XMVectorGetX(XMVector3Dot(r,u)), 0)) return fail("orthonormal basis");
    c.look(0, -100000); if (!close(c.pitch(), Camera::pitchLimit)) return fail("pitch clamp");
    c.reset(); Camera straight, diagonal;
    straight.move(0,0,1,1,false); diagonal.move(1,0,1,1,false);
    auto p = diagonal.position();
    if (!close(std::sqrt(p.x*p.x+p.y*p.y+p.z*p.z), straight.position().z)) return fail("diagonal speed");
    Camera low, high;
    for (int i=0; i<30; ++i) low.move(1,0,1,1.0f/30,false);
    for (int i=0; i<144; ++i) high.move(1,0,1,1.0f/144,false);
    if (!close(low.position().x,high.position().x) || !close(low.position().z,high.position().z)) return fail("frame rate independence");
    Camera fast; fast.move(0,0,1,1,true);
    if (!close(fast.position().z, Camera::speed * Camera::fastMultiplier)) return fail("fast speed");
    c.move(1,1,1,1,true); c.look(42,42); c.reset();
    if (!close(c.position().x,0) || !close(c.position().y,0) || !close(c.position().z,0) || !close(c.yaw(),0) || !close(c.pitch(),0)) return fail("reset");
    return 0;
}
