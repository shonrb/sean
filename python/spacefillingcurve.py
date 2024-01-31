#!/usr/bin/env python3
''' spacefillingcurve.py
    generates a gif which transfers between
    different orders of a space filling curve
    (pseudo hilbert curve)
''' 
#------------------#
# Curve generation #
#------------------#
QUAD_SIZE = 0.5
COORDS = ((-0.5, -0.5), (-0.5, 0.5), (0.5, 0.5), (0.5, -0.5))
INDICES = (0, 1, 2, 3)

def swap_indices(indices, a, b):
    new = [i for i in indices]
    new[a], new[b] = new[b], new[a]
    return tuple(new)

def hilbert_order_1(x, y, size):
    mag = size * 0.5
    x0, y0 = x-mag, y-mag
    x1, y1 = x+mag, y+mag
    return (x0, y0), (x0, y1), (x1, y1), (x1, y0)

def hilbert_order_n(order, size, coords, indices):
    curve = [coords[i] for i in indices]

    # If the curve is order 1, the max depth has been reached
    if order == 1:
        return curve

    # A nth order curve consists of 4 (n-1)th order curves connected
    # The first and last curves are flipped to connect.
    n_order = order - 1
    n_size = size * 0.5
    
    def make_seg(n_coords, n_indices):
        return hilbert_order_n(n_order, n_size, n_coords, n_indices)

    a, b, c, d = [hilbert_order_1(x, y, n_size) for x, y in curve]

    return [
        *make_seg(a, swap_indices(indices, 1, 3)),
        *make_seg(b, indices),
        *make_seg(c, indices),
        *make_seg(d, swap_indices(indices, 0, 2))
    ]

def hilbert(order):
    size    = 1
    indices = (0, 1, 2, 3)
    coords  = hilbert_order_1(0, 0, size)
    return hilbert_order_n(order, size, coords, indices)


#----------------#
# Gif generation #
#----------------#
from PIL import Image, ImageDraw, ImageFile
from random import randint

WIDTH = 600
HEIGHT = 600
ORDER_DEPTH = 7
TRANSITION_FRAMES = 40
STILL_FRAMES = 20
FG_COLOUR = (255, 255, 255)
BG_COLOUR = (0, 0, 0)

def to_screenspace(x, y):
    return (
        ( x+1) * WIDTH * 0.5, 
        (-y+1) * HEIGHT * 0.5
    )

def draw_curve(points):
    size = (WIDTH, HEIGHT)
    bg = (*BG_COLOUR, 255)
    blank = Image.new("RGBA", size, bg)
    draw = ImageDraw.Draw(blank)
    draw.line(points, fill=None, width=0)
    return blank

def divide_lines(points, order):
    ''' Divides a nth order curve so that it has the same number of 
        points as a (n+1)th order curve
    '''
    num_lines = len(points) - 1

    # Each line is divided into 4 to reflect the fact that a (n+1)th 
    # order curve consists of 4 nth order curves
    default_divisor = 4

    # Since for n>1 a nth order curve consists of 4 (n-1)th order
    # curves, if the line number is a multiple of 4^(n-1) then the
    # line is a connection between curves.
    connection_every = 4 ** (order-1)

    new = []

    for i in range(num_lines):
        sx, sy = points[i]
        ex, ey = points[i+1]

        divisor = default_divisor

        # Connections between curves require an extra point
        if (i + 1) % connection_every == 0:
            divisor += 1

        # Walk the start point towards the end
        dx = (ex - sx) / divisor
        dy = (ey - sy) / divisor

        for j in range(divisor):
            point = (
                sx + dx * j,
                sy + dy * j
            )
            new.append(point)

    new.append(points[-1])
    return new

def make_screenspace_curve(order):
    return [to_screenspace(x, y) for x, y in hilbert(order)]

def pointlist_map(points0, points1, func):
    return [
        (func(x0, x1), func(y0, y1)) 
        for (x0, y0), (x1, y1)
        in zip(points0, points1)
    ]

def get_transition_delta(start, end):
    return (end - start) / TRANSITION_FRAMES


frames = []

start_line = make_screenspace_curve(1)

for order in range(1, ORDER_DEPTH):
    # Create the still frames of the current curve
    frames += [draw_curve(start_line) for _ in range(STILL_FRAMES)]

    # Get the next curve and divide the current one so 
    # they have the same number of points
    end_line = make_screenspace_curve(order+1)
    start_line = divide_lines(start_line, order)
    
    # Get the delta for each point for each transition frame
    delta = pointlist_map(start_line, end_line, get_transition_delta)

    for transition_frame in range(TRANSITION_FRAMES + 1):
        get = lambda a, b: a + b*transition_frame
        new_line = pointlist_map(start_line, delta, get)
        img = draw_curve(new_line)
        frames.append(img)

    start_line = end_line

    print(f"{order}/{ORDER_DEPTH-1} Completed")

frames[0].save(
    "output.gif",
    save_all      = True, 
    append_images = frames[1:], 
    optimize      = False, 
    duration      = 20
)
