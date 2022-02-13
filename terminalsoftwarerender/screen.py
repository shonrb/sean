from collections import namedtuple
import math
from triangle import draw_triangle

Colour = namedtuple("Colour", ["r", "g", "b"])

class Screen:

    def __init__(self, width, height, background):
        self.background = background
        self.width = width
        self.height = height
        self.buffer = None
        self.z_buffer = None
        self.clear()


    def draw_triangle(self, tri, colour):
        put = lambda x, y, z : self.set_pixel(x, y, z, colour)
        draw_triangle(tri, put)

    def set_pixel(self, x, y, z, colour):
        d = self.z_buffer[y][x]
        x_valid = x in range(0, self.width)
        y_valid = y in range(0, self.height)

        if x_valid and y_valid and z - d > 0.00001:
            self.buffer[y][x] = colour
            self.z_buffer[y][x] = z

    def clear(self):
        def new_buf(x):
            return [
                [x for _ in range(self.width)] 
                   for _ in range(self.height)
            ]
        self.buffer = new_buf(self.background)
        self.z_buffer = new_buf(-math.inf)

    def print(self):
        to_string = lambda p : f"\033[48;2;{p.r};{p.g};{p.b}m  "

        for row in self.buffer:
            codes = [to_string(p) for p in row]
            line = "".join(codes) + "\033[0m"
            print(line)