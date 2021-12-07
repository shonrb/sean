#include "flock.hpp"
#include <random>

Flock::Boid::Boid(float x, float y, float vx, float vy, unsigned c, unsigned i)
: pos({x, y}), vel({vx, vy}), colour(c), id(i)
{}

Flock::Flock(unsigned w, unsigned h, unsigned count) : width(w), height(h)
{
    std::mt19937 rnd{};

    auto random = [&](int min, int max) {
        return std::uniform_int_distribution<>(min, max)(rnd);
    };

    for (unsigned i = 0; i < count; ++i) {
        float    x  = random(0, width);
        float    y  = random(0, height);
        unsigned c  = random(0xFF000000, 0xFFFFFFFF);
        boids.push_back(Boid(x, y, 0, 0, c, i));
    }
}

void Flock::update()
{
    update_boids_internal({});
}

void Flock::update(Vec2f attractor)
{
    update_boids_internal(attractor);
}

void Flock::update_boids_internal(std::optional<Vec2f> attractor)
{
    constexpr float SENSORY_RANGE = 200;
    constexpr float SENSORY_RANGE2 = SENSORY_RANGE*SENSORY_RANGE;
    constexpr float ALIGNMENT_FACTOR = 100.0f;
    constexpr float COHESION_FACTOR = 0.01f;
    constexpr float SEPARATION_FACTOR = 0.01f;
    constexpr float ATTRACTOR_FACTOR = 3.5f;
    constexpr float BOID_SPEED = 4.0f;

    for (auto& boid: boids) {
        // Apply velocity
        boid.pos.x += boid.vel.x;
        boid.pos.y += boid.vel.y;

        // If there is an attractor, steer towards it.
        if (attractor.has_value()) {
            auto pos = attractor.value();
            auto diff = boid.pos - pos;
            boid.vel -= diff.normal() * ATTRACTOR_FACTOR;
        }

        // If the boid leaves the valid area, move it to the other side
        if (boid.pos.x < 0)       boid.pos.x = width-1;
        if (boid.pos.y < 0)       boid.pos.y = height-1;
        if (boid.pos.x >= width)  boid.pos.x = 0;
        if (boid.pos.y >= height) boid.pos.y = 0;

        // Sum up the positions and velocities of each neighbouring boid,
        // as well as the total separation force applied
        Vec2f pos_sum{0, 0};
        Vec2f vel_sum{0, 0};
        Vec2f force_sum{0, 0};
        unsigned num_neighbours = 0;

        for (auto& other: boids) {
            // Ignore the current boid
            if (boid.id == other.id) 
                continue;
            
            // Determine if the other boid is in the boid's sensory range
            auto dist = boid.pos - other.pos;
            auto len = dist.mag2();

            if (len > SENSORY_RANGE2) 
                continue;

            // The other boid is a neighbour, save it's attributes
            num_neighbours++;
            pos_sum += other.pos;
            vel_sum += other.vel;

            // Calculate the separation force applied by it
            if (len > 0.0f) {
                auto force = dist.normal() / dist.mag();
                force_sum += force;
            }
        }
        
        // Helper function to steer by the difference between a vector 
        // and an average attribute from the boid's neighbours
        auto steer_towards_avg = [&](Vec2f sum, Vec2f from, float modifier) {
            auto avg = sum / num_neighbours;
            auto diff = (from - avg) * modifier;
            boid.vel += diff;
        };

        // Apply the three rules.
        if (num_neighbours > 0) {
            steer_towards_avg(pos_sum, boid.pos, COHESION_FACTOR);
            steer_towards_avg(vel_sum, boid.vel, ALIGNMENT_FACTOR);
            boid.vel += force_sum.normal() * SEPARATION_FACTOR;
        }

        // Finally normalise the boid's speed
        boid.vel = boid.vel.normal() * BOID_SPEED;
    }
}

