#ifndef _FLOCK_H
#define _FLOCK_H

#include <vector>
#include <optional>
#include "vec.hpp"


class Flock {
    struct Boid {
        Vec2f pos;
        Vec2f vel;
        unsigned colour;
        unsigned id;

        Boid(float x, float y, float vx, float vy, unsigned c, unsigned i);
    };

    std::vector<Boid> boids;
    unsigned width;
    unsigned height;

public:
    Flock(unsigned w, unsigned h, unsigned count);
    void update();
    void update(Vec2f attractor);

    void draw_boids(auto&& line) const
    {
        for (auto& boid : boids) {
            auto prev = boid.pos - boid.vel;
            line(
                boid.pos.x, boid.pos.y, 
                prev.x, prev.y, boid.colour);
        }
    }

private:
    void update_boids_internal(std::optional<Vec2f> attractor);
};

#endif