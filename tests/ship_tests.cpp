#include "ship.h"
#include <cmath>
#include <iostream>
using namespace DirectX;
int main() {
    auto close = [](float a, float b) { return std::fabs(a-b) < 0.001f; };
    auto fail = [](const char* s) { std::cerr << s << '\n'; return 1; };
    Ship a, b;
    for (int i=0; i<30; ++i) a.update(1,0,0,0,false,1.0f/30);
    for (int i=0; i<144; ++i) b.update(1,0,0,0,false,1.0f/144);
    if (!close(a.position().z, Ship::speed) || !close(a.position().z,b.position().z)) return fail("fixed interval movement");
    Ship diagonal; diagonal.update(1,0,0,1,false,1);
    auto p = diagonal.position();
    if (!close(std::sqrt(p.y*p.y+p.z*p.z), Ship::speed)) return fail("normalized movement");
    Ship mouse; mouse.look(100, -200);
    if (!close(mouse.yaw(), 100 * Ship::mouseSensitivity) ||
        !close(mouse.pitch(), 200 * Ship::mouseSensitivity)) return fail("mouse direction and sensitivity");
    Ship split; split.look(40, -80); split.look(60, -120);
    if (!close(mouse.yaw(), split.yaw()) || !close(mouse.pitch(), split.pitch())) return fail("mouse displacement independent of frame count");
    mouse.look(0, -100000); if (!close(mouse.pitch(), Ship::pitchLimit)) return fail("mouse pitch clamp");
    mouse.look(0, 200000); if (!close(mouse.pitch(), -Ship::pitchLimit)) return fail("mouse negative pitch clamp");
    Ship turn; turn.update(0,1,1,0,false,0.5f);
    if (!close(turn.yaw(), Ship::turnRate/2) || !close(turn.pitch(), Ship::turnRate/2)) return fail("turn rates");
    auto parts = turn.objects();
    XMFLOAT3 noseDirection{};
    auto turnPosition = turn.position();
    XMStoreFloat3(&noseDirection, XMVector3Normalize(XMVectorSubtract(XMLoadFloat3(&parts[1].translation), XMLoadFloat3(&turnPosition))));
    XMFLOAT3 f{}; XMStoreFloat3(&f, turn.forward());
    if (!close(noseDirection.x,f.x) || !close(noseDirection.y,f.y) || !close(noseDirection.z,f.z)) return fail("ship orientation");
    XMFLOAT3 target{}; XMStoreFloat3(&target, XMVector3TransformCoord(XMLoadFloat3(&turnPosition), turn.chaseView()));
    if (!close(target.x,0) || !close(target.y,0) || target.z <= 0) return fail("chase target");
    turn.update(1,0,0,0,true,0); turn.reset();
    if (!close(turn.position().z,0) || !close(turn.yaw(),0) || !close(turn.pitch(),0)) return fail("reset");
    return 0;
}

