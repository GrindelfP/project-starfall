#pragma once
#include "scene.h"
#include "collision.h"
#include <array>
#include <cstdint>
#include <vector>

// CPU-only deterministic obstacle data. All cube corners fit inside each sphere.
class AsteroidField {
public:
    struct Rock { DirectX::XMFLOAT3 center; float radius; DirectX::XMFLOAT3 rotation; DirectX::XMFLOAT4 color; };
    static constexpr std::size_t count = 64;
    static constexpr float shipRadius = 1.9f;
    static constexpr float spawnClearance = 10.0f;
    static constexpr std::uint32_t defaultSeed = 0x53544152u;

    explicit AsteroidField(std::uint32_t seed = defaultSeed) { reset(seed); }
    void reset(std::uint32_t seed) {
        seed_ = seed;
        std::uint32_t state = seed;
        auto random = [&]() { state = state * 1664525u + 1013904223u; return float(state >> 8) / 16777216.0f; };
        auto range = [&](float a, float b) { return a + (b-a)*random(); };
        for (auto& rock : rocks_) {
            // Rejection sampling has a fixed field domain and a very small excluded spawn volume.
            do { rock.center = {range(-40,40), range(-19,19), range(5,85)}; rock.radius = range(1.2f,3.5f); }
            while (overlaps(rock.center, rock.radius + spawnClearance, {0,0,0}, shipRadius));
            rock.rotation = {range(0,6.28f), range(0,6.28f), range(0,6.28f)};
            const float shade = range(0.25f,0.52f);
            rock.color = {shade, shade*0.83f, shade*0.73f, 1};
        }
    }
    static bool overlaps(DirectX::XMFLOAT3 a, float ar, DirectX::XMFLOAT3 b, float br) {
        const float x=a.x-b.x, y=a.y-b.y, z=a.z-b.z, r=ar+br;
        return x*x+y*y+z*z <= r*r;
    }
    // Parts are at the final pose. Rotation during a sweep is approximated by that pose.
    bool contact(const std::vector<scene::Object>& parts, DirectX::XMFLOAT3 previousPosition,
                 DirectX::XMFLOAT3 position) const {
        const DirectX::XMFLOAT3 displacement{position.x-previousPosition.x,
            position.y-previousPosition.y, position.z-previousPosition.z};
        for (const auto& rock : rocks_) for (const auto& box : rockObjects(rock)) {
            for (auto part : parts) {
                part.translation.x-=displacement.x;
                part.translation.y-=displacement.y;
                part.translation.z-=displacement.z;
                if (collision::sweptBoxes(part, displacement, box)) return true;
            }
        }
        return false;
    }
    static std::array<scene::Object, 2> rockObjects(const Rock& rock) {
        const auto r = rock.radius;
        return {{{{1.25f*r,0.95f*r,0.95f*r}, rock.rotation, rock.center, rock.color},
            {{0.9f*r,1.2f*r,0.9f*r},
                {rock.rotation.x+0.55f,rock.rotation.y+0.65f,rock.rotation.z+0.4f}, rock.center,
                {rock.color.x*0.8f,rock.color.y*0.8f,rock.color.z*0.8f,1}}}};
    }
    std::uint32_t seed() const { return seed_; }
    const std::array<Rock, count>& rocks() const { return rocks_; }
    std::vector<scene::Object> objects() const {
        std::vector<scene::Object> result;
        result.reserve(count*2 + 100);
        for (const auto& rock : rocks_) {
            const auto boxes = rockObjects(rock);
            result.insert(result.end(), boxes.begin(), boxes.end());
        }
        // Stationary sparse star points and colored wayfinding lights, sharing the cube mesh.
        std::uint32_t state = 0x91e10da5u;
        auto random = [&]() { state = state * 1664525u + 1013904223u; return float(state >> 8) / 16777216.0f; };
        for (int i=0; i<96; ++i) {
            const float x = (random()*2-1)*65, y = (random()*2-1)*40, z = 12+random()*75;
            const float size = 0.09f + random()*0.12f;
            result.push_back({{size,size,size},{0,0,0},{x,y,z},{0.7f,0.85f,1,1}});
        }
        for (int i=0; i<4; ++i) {
            const float x = (i%2 ? 1.0f : -1.0f)*18, z = 22.0f + 19.0f*i;
            result.push_back({{0.5f,1.8f,0.5f},{0,0,0},{x,-8,z},{0.08f,0.85f,0.95f,1}});
        }
        return result;
    }
private:
    std::uint32_t seed_{};
    std::array<Rock, count> rocks_{};
};

