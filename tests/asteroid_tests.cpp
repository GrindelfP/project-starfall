#include "asteroid_field.h"
#include "ship.h"
#include <iostream>
using namespace DirectX;
int main() {
    auto fail=[](const char* message) { std::cerr << message << '\n'; return 1; };
    using P = XMFLOAT3;
    scene::Object a{{2,2,2},{0,0,0},{0,0,0},{}};
    scene::Object b=a;
    b.translation={2,0,0};
    if (!collision::sweptBoxes(a,{0,0,0},b)) return fail("touching faces");
    b.translation={2.001f,0,0};
    if (collision::sweptBoxes(a,{0,0,0},b)) return fail("separated faces");
    b.translation={1.9f,0,0};
    if (!collision::sweptBoxes(a,{0,0,0},b)) return fail("overlap");
    b.translation={0,0,10};
    if (!collision::sweptBoxes(a,{0,0,20},b)) return fail("swept impact");
    b.translation={2.01f,0,10};
    if (collision::sweptBoxes(a,{0,0,20},b)) return fail("swept near miss");
    for (float angle : {0.0f, 0.4f, 1.1f}) {
        a.rotation={0,0,angle}; b.rotation={0,0,angle};
        // Translate in the common local x direction: touching and a corner miss.
        b.translation={2*std::cos(angle),2*std::sin(angle),0};
        if (!collision::sweptBoxes(a,{0,0,0},b)) return fail("rotated touch");
        b.translation={2.01f*std::cos(angle),2.01f*std::sin(angle),2.01f};
        if (collision::sweptBoxes(a,{0,0,0},b)) return fail("rotated corner miss");
    }
    // Actual ship nose and rock box, separated along local x, at several orientations.
    Ship ship;
    for (float angle : {0.0f, 0.5f, 1.2f}) {
        auto nose=ship.objects()[1];
        nose.rotation={0,0,angle}; nose.translation={0,0,0};
        AsteroidField::Rock rock{{0,0,0},2,{0,0,angle},{}};
        auto box=AsteroidField::rockObjects(rock)[0];
        const float limit=(nose.scale.x+box.scale.x)/2;
        box.translation={(limit+0.01f)*std::cos(angle),(limit+0.01f)*std::sin(angle),0};
        if (collision::sweptBoxes(nose,{0,0,0},box)) return fail("ship rock side miss");
        box.translation={(limit-0.01f)*std::cos(angle),(limit-0.01f)*std::sin(angle),0};
        if (!collision::sweptBoxes(nose,{0,0,0},box)) return fail("ship rock impact");
    }
    for (unsigned seed : {0u,1u,42u,AsteroidField::defaultSeed,0xffffffffu}) {
        AsteroidField x(seed), y(seed), z(seed+1);
        if (x.seed()!=seed || x.contact(ship.objects(),ship.position(),ship.position())) return fail("safe spawn/seed");
        bool different=false;
        for (std::size_t i=0; i<x.count; ++i) {
            const auto& r=x.rocks()[i]; const auto& s=y.rocks()[i]; const auto& t=z.rocks()[i];
            if (r.center.x!=s.center.x || r.center.y!=s.center.y || r.center.z!=s.center.z || r.radius!=s.radius || r.rotation.x!=s.rotation.x || r.rotation.y!=s.rotation.y || r.rotation.z!=s.rotation.z) return fail("reproducibility");
            if (r.center.x!=t.center.x || r.center.y!=t.center.y || r.center.z!=t.center.z) different=true;
            if (AsteroidField::overlaps(r.center,r.radius+AsteroidField::spawnClearance,{0,0,0},AsteroidField::shipRadius)) return fail("spawn clearance");
            auto parts=ship.objects();
            for (auto& part : parts) { part.translation.x+=r.center.x; part.translation.y+=r.center.y; part.translation.z+=r.center.z; }
            if (!x.contact(parts,r.center,r.center)) return fail("rock contact");
        }
        if (!different) return fail("different seed");
        x.reset(seed+1); x.reset(seed);
        if (x.rocks()[0].center.x!=y.rocks()[0].center.x) return fail("reset layout");
        if (x.objects().size()!=x.count*2+100) return fail("object count");
    }
    return 0;
}


