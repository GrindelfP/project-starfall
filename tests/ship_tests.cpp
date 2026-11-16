#include "ship.h"
#include <cmath>
#include <iostream>
using namespace DirectX;
int main() {
    auto close = [](float a, float b) { return std::fabs(a-b) < 0.001f; };
    auto fail = [](const char* s) { std::cerr << s << '\n'; return 1; };
    Ship a, b;
    for (int i=0; i<30; ++i) a.update(1,0,0,0,0,false,false,1.0f/30);
    for (int i=0; i<144; ++i) b.update(1,0,0,0,0,false,false,1.0f/144);
    if (!close(a.velocity().z, Ship::acceleration) || !close(a.position().z, Ship::acceleration/2) ||
        !close(a.position().z,b.position().z)) return fail("fixed interval acceleration");
    a.update(0,0,0,0,0,false,false,1);
    if (!close(a.velocity().z, Ship::acceleration) || !close(a.position().z, Ship::acceleration*1.5f)) return fail("coasting");
    a.look(XM_PIDIV2 / Ship::mouseSensitivity, 0);
    a.update(0,0,0,0,0,false,false,1);
    if (!close(a.velocity().x,0) || !close(a.velocity().z,Ship::acceleration)) return fail("turn preserves velocity");
    a.look(-XM_PIDIV2 / Ship::mouseSensitivity, 0);
    a.update(-1,0,0,0,0,false,false,1);
    if (!close(a.velocity().z,0)) return fail("reverse thrust brakes");
    Ship diagonal; diagonal.update(1,0,0,0,1,false,false,1);
    auto v = diagonal.velocity();
    if (!close(std::sqrt(v.y*v.y+v.z*v.z), Ship::acceleration)) return fail("normalized thrust");
    Ship boosted; boosted.update(1,0,0,0,0,true,false,1);
    if (!close(boosted.velocity().z, Ship::acceleration * Ship::boost)) return fail("boost acceleration");
    boosted.update(1,0,0,0,0,true,false,10);
    if (!close(boosted.velocity().z, Ship::maxSpeed)) return fail("speed cap");
    boosted.reset(); boosted.update(0,0,0,0,0,false,false,1);
    if (!close(boosted.position().z,0) || !close(boosted.velocity().z,0)) return fail("reset clears velocity");
    Ship mouse; mouse.look(100, -200);
    if (!close(mouse.yaw(), 100 * Ship::mouseSensitivity) ||
        !close(mouse.pitch(), 200 * Ship::mouseSensitivity)) return fail("mouse direction and sensitivity");
    Ship split; split.look(40, -80); split.look(60, -120);
    if (!close(mouse.yaw(), split.yaw()) || !close(mouse.pitch(), split.pitch())) return fail("mouse displacement independent of frame count");
    mouse.look(0, -100000); if (!close(mouse.pitch(), Ship::pitchLimit)) return fail("mouse pitch clamp");
    mouse.look(0, 200000); if (!close(mouse.pitch(), -Ship::pitchLimit)) return fail("mouse negative pitch clamp");
    Ship turn; turn.update(0,0,1,1,0,false,false,0.5f);
    if (!close(turn.yaw(), Ship::turnRate/2) || !close(turn.pitch(), Ship::turnRate/2)) return fail("turn rates");
    auto parts = turn.objects();
    XMFLOAT3 noseDirection{};
    auto turnPosition = turn.position();
    XMStoreFloat3(&noseDirection, XMVector3Normalize(XMVectorSubtract(XMLoadFloat3(&parts[1].translation), XMLoadFloat3(&turnPosition))));
    XMFLOAT3 f{}; XMStoreFloat3(&f, turn.forward());
    if (!close(noseDirection.x,f.x) || !close(noseDirection.y,f.y) || !close(noseDirection.z,f.z)) return fail("ship orientation");
    XMFLOAT3 target{}; XMStoreFloat3(&target, XMVector3TransformCoord(XMLoadFloat3(&turnPosition), turn.chaseView()));
    if (!close(target.x,0) || !close(target.y,0) || target.z <= 0) return fail("chase target");
    turn.update(1,0,0,0,0,true,false,0); turn.reset();
    if (!close(turn.position().z,0) || !close(turn.yaw(),0) || !close(turn.pitch(),0)) return fail("reset");
    Ship lateral; lateral.look(XM_PIDIV2 / Ship::mouseSensitivity, 0);
    lateral.update(0,1,0,0,0,false,false,1);
    if (!close(lateral.velocity().z,-Ship::acceleration) || !close(lateral.yaw(),XM_PIDIV2)) return fail("rotated lateral translation");
    Ship combined; combined.update(1,1,0,0,1,false,false,1);
    auto cv = combined.velocity();
    if (!close(std::sqrt(cv.x*cv.x+cv.y*cv.y+cv.z*cv.z),Ship::acceleration)) return fail("three axis normalization");
    for (int axis=0; axis<5; ++axis) {
        Ship braking;
        braking.update(axis==0 ? 1.0f : axis==1 ? -1.0f : axis==4 ? 1.0f : 0.0f,
            axis==2 || axis==4 ? 1.0f : 0.0f,0,0,axis==3 || axis==4 ? 1.0f : 0.0f,false,false,1);
        auto before = braking.velocity();
        braking.update(0,0,0,0,0,false,true,0.1f);
        auto after = braking.velocity();
        float oldSpeed = std::sqrt(before.x*before.x+before.y*before.y+before.z*before.z);
        float newSpeed = std::sqrt(after.x*after.x+after.y*after.y+after.z*after.z);
        if (!close(newSpeed,oldSpeed-Ship::brakeDeceleration*0.1f) ||
            before.x*after.x+before.y*after.y+before.z*after.z <= 0) return fail("braking direction and magnitude");
        braking.update(0,0,0,0,0,false,false,1);
        auto released = braking.velocity();
        if (!close(released.x,after.x) || !close(released.y,after.y) || !close(released.z,after.z)) return fail("brake release");
        braking.update(0,0,0,0,0,false,true,10);
        auto stopped = braking.velocity();
        if (!close(stopped.x,0) || !close(stopped.y,0) || !close(stopped.z,0)) return fail("brake stop without reversal");
        braking.update(0,0,0,0,0,false,true,1);
        if (!close(braking.velocity().z,0)) return fail("brake at zero");
    }
    Ship coarse, fine;
    coarse.update(1,0,0,0,0,false,false,2); fine.update(1,0,0,0,0,false,false,2);
    coarse.update(0,0,0,0,0,false,true,0.5f);
    for (int i=0; i<50; ++i) fine.update(0,0,0,0,0,false,true,0.01f);
    if (!close(coarse.velocity().z,fine.velocity().z) || !close(coarse.position().z,fine.position().z)) return fail("brake timestep consistency");
    Ship simultaneous; simultaneous.update(1,0,0,0,0,false,false,2);
    simultaneous.update(1,0,0,0,0,false,true,0.5f);
    if (!close(simultaneous.velocity().z,4.5f)) return fail("thrust before brake");
    return 0;
}

