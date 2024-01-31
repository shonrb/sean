#ifndef _VEC_H
#define _VEC_H

#include <cmath>

struct Vec2f {
    float x, y;

#define DO_OP(op)                                 \
    Vec2f operator op (const Vec2f& rhs) const    \
        { return Vec2f {x op rhs.x, y op rhs.y}; }\
    void operator op##=(const Vec2f& rhs)         \
        { x op##= rhs.x; y op##= rhs.y; }         \
    Vec2f operator op (float rhs) const           \
        { return Vec2f {x op rhs, y op rhs}; }    \
    void operator op##=(float rhs)                \
        { x op##= rhs; y op##= rhs; }         

    DO_OP(+)
    DO_OP(-)
    DO_OP(*)
    DO_OP(/)

    float mag2() const
    {
        return x*x + y*y;
    }

    float mag() const 
    {
        return std::sqrt(mag2());
    }

    Vec2f normal() const
    {
        float invmag = 1 / mag();
        return Vec2f{x * invmag, y * invmag};
    }  
};

#endif