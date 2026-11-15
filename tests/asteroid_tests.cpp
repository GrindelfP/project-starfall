#include "asteroid_field.h"
#include <iostream>
int main() {
    auto fail=[](const char* message) { std::cerr << message << '\n'; return 1; };
    using P = DirectX::XMFLOAT3;
    if (!AsteroidField::overlaps(P{0,0,0},1,P{2,0,0},1)) return fail("tangent contact");
    if (AsteroidField::overlaps(P{0,0,0},1,P{2.001f,0,0},1)) return fail("separation");
    if (!AsteroidField::overlaps(P{0,0,0},1,P{0,0,0},1)) return fail("coincident contact");
    for (unsigned seed : {0u,1u,42u,AsteroidField::defaultSeed,0xffffffffu}) {
        AsteroidField a(seed), b(seed), c(seed+1);
        if (a.seed()!=seed || a.contact({0,0,0})) return fail("safe spawn/seed");
        bool different=false;
        for (std::size_t i=0; i<a.count; ++i) {
            const auto& x=a.rocks()[i]; const auto& y=b.rocks()[i]; const auto& z=c.rocks()[i];
            if (x.center.x!=y.center.x || x.center.y!=y.center.y || x.center.z!=y.center.z || x.radius!=y.radius) return fail("reproducibility");
            if (x.center.x!=z.center.x || x.center.y!=z.center.y || x.center.z!=z.center.z) different=true;
            if (AsteroidField::overlaps(x.center,x.radius+AsteroidField::spawnClearance,{0,0,0},AsteroidField::shipRadius)) return fail("spawn clearance");
            if (!a.contact(x.center)) return fail("rock contact");
        }
        if (!different) return fail("different seed");
        a.reset(seed+1); a.reset(seed);
        if (a.rocks()[0].center.x!=b.rocks()[0].center.x) return fail("reset layout");
        if (a.objects().size()!=a.count*2+100) return fail("object count");
    }
    return 0;
}
