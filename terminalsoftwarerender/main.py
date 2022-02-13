#!/usr/bin/env python3
from screen import Screen, Colour
from collections import namedtuple
from random import randint
import math
import numpy as np
import time
import os

Triangle = namedtuple("Triangle", ["verts", "colour"])

BUFFER_WIDTH = 50
BUFFER_HEIGHT = 50
ASPECT = BUFFER_HEIGHT / BUFFER_WIDTH
FOV = 1.56 # in radians
NEAR_Z = 0.5
FAR_Z = 1000.0

PROJECTION_MATRIX = np.array([
    [ASPECT * FOV, 0,   0,                                    0],
    [0,            FOV, 0,                                    0],
    [0,            0,   FAR_Z / (FAR_Z - NEAR_Z),             1],
    [0,            0,   (-FAR_Z * NEAR_Z) / (FAR_Z - NEAR_Z), 0]
])

VERTICES = [
    [-1.0, -1.0,  1.0], # 0
    [ 1.0, -1.0,  1.0], # 1
    [-1.0,  1.0,  1.0], # 2
    [ 1.0,  1.0,  1.0], # 3
    [-1.0, -1.0, -1.0], # 4
    [ 1.0, -1.0, -1.0], # 5
    [-1.0,  1.0, -1.0], # 6
    [ 1.0,  1.0, -1.0], # 7
]

INDICES = [
    [2, 6, 7], [2, 3, 7], # Top
    [0, 4, 5], [0, 1, 5], # Bottom
    [0, 2, 6], [0, 4, 6], # Left
    [1, 3, 7], [1, 5, 7], # Right
    [0, 2, 3], [0, 1, 3], # Front
    [4, 6, 7], [4, 5, 7], # Back
]

def randcol():
    return Colour(randint(0, 255), randint(0, 255), randint(0, 255))

def make_tri(verts, colour):
    triadj = [np.array(v) for v in verts]
    return Triangle(triadj, colour)

def rotation_matrix(a, b, y):
    ca, cb, cy = (math.cos(i) for i in (a, b, y))
    sa, sb, sy = (math.sin(i) for i in (a, b, y))
    sab = sa * sb
    casb = ca * sb
    return np.array([
        [cb*cy, sab*cy - ca*sy, casb*cy + sa*sy, 0],
        [cb*sy, sab*sy + ca*cy, casb*sy - sa*cy, 0],
        [-sb,   sa*cb,          ca*cb,           0],
        [0,     0,              0,               1]
    ])

def multiply_mat4_vec3(mat, vec):
    res = np.sum(np.array([*vec, 1]) * mat, axis=1)
    w = res[3]
    if w != 0:
        res /= w
    return res[:3]

def main():
    screen = Screen(BUFFER_WIDTH, BUFFER_HEIGHT, Colour(0, 0, 0))
    theta = 0
    model = [
        make_tri([VERTICES[i] for i in tri], randcol())
        for tri in INDICES
    ]

    for i in range(200):
        os.system("clear")
        for tri in model:
            verts = []
            for vert in tri.verts:
                rotmat = rotation_matrix(theta, theta * 0.5, theta * 0.25)
                rotated = multiply_mat4_vec3(rotmat, vert)
                translated = rotated + np.array([0, 0, -6])
                proj = multiply_mat4_vec3(PROJECTION_MATRIX, translated)
                
                verts.append(np.array([
                    float((proj[0] + 1) * 0.5 * BUFFER_WIDTH),
                    float((proj[1] + 1) * 0.5 * BUFFER_HEIGHT),
                    proj[2]
                ]))
            screen.draw_triangle(verts, tri.colour)
        screen.print()
        theta += 0.05
        screen.clear()
        time.sleep(0.08)

if __name__ == "__main__":
    main()