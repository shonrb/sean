#include "verlet_system.h"
#include <cmath>
#include <tuple>

constexpr float FLOOR_HEIGHT = 0.11;

void VerletSystem::add_point(float x, float y, float z, bool fixed)
{
    points.push_back(VerletPointMass{
        x, y, z, x, y, z, fixed});
}

void VerletSystem::add_constraint(unsigned index0, unsigned index1)
{
    auto p0 = points[index0];
    auto p1 = points[index1];

    // Set the ideal distance to the distance between the given points
    float dx = p1.x - p0.x;
    float dy = p1.y - p0.y;
    float dz = p1.z - p0.z;
    float ideal_length = sqrtf(dx*dx + dy*dy + dz*dz);

    constraints.push_back(VerletConstraint{
        index0, index1, ideal_length
    });
}

void VerletSystem::update()
{
    constexpr int ITERATIONS = 5;
    update_point_masses();
    for (int _ = 0; _ < ITERATIONS; ++_)
        update_constraints();
}

void VerletSystem::update_grabbed_point(float x, float y, float z)
{
    if (grabbed_index != NOT_GRABBED)
    {
        auto& point = points[grabbed_index];
        point.x = x;
        point.y = std::max(y, FLOOR_HEIGHT);
        point.z = z;
    }
}


bool VerletSystem::grab_nearby_point(float x, float y, float z)
{
    constexpr float MAX_DIST = 0.2;

    for (int i = 0; i < points.size(); ++i)
    {
        auto& point = points[i];
        float dx, dy, dz, distance_squared;
        dx = x - point.x;
        dy = y - point.y;
        dz = z - point.z;
        distance_squared = dx*dx + dy*dy + dz*dz;

        if (distance_squared < MAX_DIST*MAX_DIST)
        {
            grabbed_index = i;
            return true;
        }
    }
    return false;
}

void VerletSystem::ungrab_point() 
{ 
    grabbed_index = NOT_GRABBED; 
}

void VerletSystem::update_point_masses()
{
    constexpr float GRAVITY  = 0.006;
    constexpr float FRICTION = 0.95;
    constexpr float RESTITUTION = 0.2;  
    constexpr float MAX_VEL = 2.0;
    for (auto& point : points)
    {
        if (point.fixed)
            continue;

        // Get velocity
        float vx = std::clamp(point.x - point.px,           -MAX_VEL, MAX_VEL);
        float vy = std::clamp(point.y - point.py - GRAVITY, -MAX_VEL, MAX_VEL);
        float vz = std::clamp(point.z - point.pz,           -MAX_VEL, MAX_VEL);

        // Update prev pos
        std::tie(point.px, point.py, point.pz) = std::make_tuple(
            point.x, point.y, point.z);

        point.y += vy;
        // Check for collision with ground
        if (point.y <= FLOOR_HEIGHT)
        {
            // Reverse Y and apply friction to X & Z
            point.y = FLOOR_HEIGHT;
            point.py = point.y + vy * RESTITUTION;
            vx *= FRICTION;
            vz *= FRICTION;
        }
        point.x += vx;
        point.z += vz;
    }
}

void VerletSystem::update_constraints()
{
    for (auto& constraint : constraints)
    {
        auto& p0 = points[constraint.index0];
        auto& p1 = points[constraint.index1];

        // Find the distance that the points must move to
        // maintain the constraint's distance.
        float dx, dy, dz, distance, difference, mult;
        dx = p1.x - p0.x;
        dy = p1.y - p0.y;
        dz = p1.z - p0.z;
        distance = sqrt(dx*dx + dy*dy + dz*dz);
        difference = constraint.ideal_length - distance;
        mult = (difference / distance) * 0.5f;
        
        float ox = dx*mult;
        float oy = dy*mult;
        float oz = dz*mult;
        
        // Correct the points
        if (!p0.fixed)
        {
            p0.x -= ox;
            p0.y -= oy;
            p0.z -= oz;
        }

        if (!p1.fixed)
        {
            p1.x += ox;
            p1.y += oy;
            p1.z += oz;
        }
    }
}