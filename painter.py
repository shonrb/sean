''' painter.py 
    A beam search program to reproduce images with rectangles.
    Usage: python3 painter.py [image]
'''
import os
import sys
import random
import math
import numpy as np
from collections import namedtuple
from PIL import Image, ImageDraw, ImageFile

ITERATIONS = 400
NUM_RANDOM_RECTS = 200
HILL_CLIMBING_STEPS = 200
CLIMB_MOD = 5
COLOUR_SAMPLE_SIZE = (8, 8)
ALPHA_MIN = 100
ALPHA_MAX = 255
SAVE_EVERY = 100

Attempt = namedtuple("Attempt", ["points", "colour", "image", "distance"])

def draw_optimised_rect(base, target, pallete):
    improved = False
    closest_so_far = base

    # Tries the given shape and replaces the closest if it is closer
    def draw_and_test(points, colour):
        # Draw onto a fully transparent image
        overlay = Image.new("RGBA", base.image.size, (0,0,0,0))
        draw    = ImageDraw.Draw(overlay)
        draw.rectangle(points, colour)

        # Combine the overlay with the base and calculate the new distance
        new_image    = Image.alpha_composite(base.image, overlay)
        as_array     = np.array(new_image, dtype=np.uint32)
        new_distance = np.square(target - as_array).sum()

        nonlocal closest_so_far, improved
        if new_distance < closest_so_far.distance:
            closest_so_far = Attempt(points, colour, new_image, new_distance)
            improved = True

    for _ in range(NUM_RANDOM_RECTS):
        # Calculate a random colour and set of points
        alpha   = random.randint(ALPHA_MIN, ALPHA_MAX)
        r, g, b = random.choice(pallete)
        colour  = (r, g, b, alpha)

        w, h = base.image.size
        points  = [
            (random.randint(0, w), random.randint(0, h)) 
            for _ in range(2)]
        draw_and_test(points, colour)

    if improved:
        for _ in range(HILL_CLIMBING_STEPS):
            # Modify the points very slightly to find the most optimal version
            change = lambda x : x + random.randint(-CLIMB_MOD, CLIMB_MOD)
            points = [(change(x), change(y)) for x, y in closest_so_far.points]
            draw_and_test(points, closest_so_far.colour)

    return closest_so_far, improved


def main():
    if len(sys.argv) != 2:
        print("Usage: [python] geneticalg.py [image name]")
        return

    path          = os.path.join(sys.path[0], "")
    target_image  = Image.open(path + sys.argv[1]).convert("RGBA")
    width, height = target_image.size
    target        = np.array(target_image, dtype=np.uint32)

    # Get the dominant colours in the image.
    # Image.getcolors() returns a list of items in format 
    # (count, (r, g, b, a)), so convert it to a list of (r, g, b)
    downsized = target_image.resize(COLOUR_SAMPLE_SIZE, resample=Image.NEAREST)
    pallete = [c[1][:3] for c in downsized.getcolors()]

    # Get the background colour (average of pallete)
    r, g, b = tuple(map(lambda x: int(np.mean(x)), zip(*pallete)))
    background_color = (r, g, b, 255)

    # Set up the initial image with a blank image and infinite distance
    blank   = Image.new("RGBA", target_image.size, background_color)
    closest = Attempt(None, None, blank, math.inf)

    for generation in range(ITERATIONS):
        closest, improved = draw_optimised_rect(closest, target, pallete)
        msg = "Distance decreased!" if improved else "Distance unchanged!"
        print(f"Generation {generation}: {msg} Dist: {closest.distance}")

        if generation % SAVE_EVERY == 0:
            closest.image.save(f"{path}new{generation}.png")

    closest.image.save(path + "new_final.png")


if __name__ == "__main__":
    main()
