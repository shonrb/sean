#ifndef __VERLET_H_
#define __VERLET_H_

#include <vector>

constexpr int NOT_GRABBED = -1;

class VerletSystem
{
    struct VerletPointMass
    {
        float x, y, z;
        float px, py, pz;
        bool fixed;
    };

    struct VerletConstraint
    {
        unsigned index0, index1;
        float ideal_length;
    };

    std::vector<VerletPointMass> points;
    std::vector<VerletConstraint> constraints;
    std::vector<unsigned> element_array;
    int grabbed_index = NOT_GRABBED;

public:
    void add_point(float x, float y, float z, bool fixed=false);
    void add_constraint(unsigned index0, unsigned index1);
    void update();

    void update_grabbed_point(float x, float y, float z);
    bool grab_nearby_point(float x, float y, float z);
    void ungrab_point();

    void draw_points(auto&& draw) const
    {
        for (const auto& point : points) draw(point.x, point.y, point.z);
    }

private:
    void update_point_masses();
    void update_constraints();
};

#endif