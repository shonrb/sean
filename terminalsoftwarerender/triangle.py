class _Slope2D:

    def __init__(self, begin, end):
        self.current = begin[0]
        num_steps = end[1] - begin[1]
        inv_num_steps = 1.0 / num_steps if num_steps != 0 else 0
        self.step = (end[0] - begin[0]) * inv_num_steps

    def next(self):
        ret = self.current
        self.current += self.step
        return ret

class _Slope3D:

    def __init__(self, begin, end):
        self.xy = _Slope2D(begin, end)
        self.zy = _Slope2D(begin[::-1], end[::-1])

    def next(self):
        return self.xy.next(), self.zy.next()


def draw_triangle(tri, put_pixel):
    verts = sorted(tri, key=lambda x : (x[1], x[0]))
    p0, p1, p2 = verts

    longside = _Slope3D(p0, p2)

    def draw_section(top, bottom):
        shortside = _Slope3D(top, bottom)
        y0 = int(top[1])
        y1 = int(bottom[1])

        for y in range(y0, y1):
            start = shortside.next()
            end   = longside.next()
            # Draw from left to right
            start, end = sorted([start, end])
            x0 = int(start[0])
            x1 = int(end[0])
            # Interpolate the z coordinate
            z_slope = _Slope2D(start[::-1], end[::-1])

            for x in range(x0, x1):
                z = z_slope.next()
                put_pixel(x, y, z)

    draw_section(p0, p1)
    draw_section(p1, p2)