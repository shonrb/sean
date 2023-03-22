#!/usr/bin/env python3
import random as rnd
import numpy as np
from PIL import Image, ImageDraw
from dataclasses import dataclass

NUM_STEPS               = 500
NUM_HILL_CLIMBING_STEPS = 1000
COLOUR_MAX_CHANGE       = 30
POSITION_MAX_CHANGE     = 30
A_MIN                   = 100
A_MAX                   = 255
VERBOSE_OUTPUT          = True
IMAGE_NAME              = "image.png" # target image

@dataclass
class Rect:
    ax: int
    ay: int
    bx: int
    by: int
    r:  int
    g:  int
    b:  int
    a:  int

    def copy(self):
        return Rect(**self.__dict__)

    def mutate_colour(self):
        self.r = clamp(self.r + random_delta(COLOUR_MAX_CHANGE), 0, 255)
        self.g = clamp(self.g + random_delta(COLOUR_MAX_CHANGE), 0, 255)
        self.b = clamp(self.b + random_delta(COLOUR_MAX_CHANGE), 0, 255)
        self.a = clamp(self.a + random_delta(COLOUR_MAX_CHANGE), A_MIN, A_MAX)

    def mutate_position(self, mx, my):
        self.ax = clamp(self.ax + random_delta(POSITION_MAX_CHANGE), 0, mx)
        self.ay = clamp(self.ay + random_delta(POSITION_MAX_CHANGE), 0, my)
        self.bx = clamp(self.bx + random_delta(POSITION_MAX_CHANGE), 0, mx)
        self.by = clamp(self.by + random_delta(POSITION_MAX_CHANGE), 0, my)

def random_delta(x):
    return rnd.randint(-x, +x)

def clamp(v, min_v, max_v):
    return max(min_v, min(max_v, v))

def get_distance(target, image):
    as_array = np.array(image, dtype=np.uint32)
    return np.square(target - as_array).sum()

def draw(current, target, rect):
    # Draw the rect to a transparent image
    colour  = (rect.r, rect.g, rect.b, rect.a)
    points  = [(rect.ax, rect.ay), (rect.bx, rect.by)]
    overlay = Image.new("RGBA", current.size, (0, 0, 0, 0))
    draw    = ImageDraw.Draw(overlay)
    draw.rectangle(points, colour)

    # Combine the overlay with the base and calculate the new distance
    new_image    = Image.alpha_composite(current, overlay)
    new_distance = get_distance(target, new_image)
    return new_image, new_distance

def next_state(current, target):
    width, height = current.size

    # Draw a random rectangle
    rect = Rect(
        ax = rnd.randint(0, width), 
        ay = rnd.randint(0, width), 
        bx = rnd.randint(0, width), 
        by = rnd.randint(0, width), 
        r  = rnd.randint(0, 255),  
        g  = rnd.randint(0, 255),  
        b  = rnd.randint(0, 255),  
        a  = rnd.randint(A_MIN, A_MAX)
    )
    image, distance = draw(current, target, rect)

    # Optimise the rectangle 
    col = lambda r : r.mutate_colour()
    pos = lambda r : r.mutate_position(width-1, height-1)

    for mutate in [col, pos]:
        for i in range(NUM_HILL_CLIMBING_STEPS):
            # Copy the rect and mutate it
            copy = rect.copy()
            mutate(copy)
            mutation_image, mutation_distance = draw(current, target, copy)
            
            # If it's better, accept it
            if mutation_distance < distance:
                image    = mutation_image
                distance = mutation_distance
                rect     = copy

    return image, distance

def main():
    target_image  = Image.open(IMAGE_NAME).convert("RGBA")
    target        = np.array(target_image, dtype=np.uint32)

    # Start with a blank image and infinite distance
    image    = Image.new("RGBA", target_image.size, (255, 255, 255, 255))
    distance = get_distance(target, image)
    image.save("new0.png")

    for step in range(NUM_STEPS):
        print(f"step {step}")
        print(f"starting with distance of {distance}")
 
        new_image, new_distance = next_state(image, target)

        if new_distance < distance:
            print(f"distance decreased by {distance-new_distance}")
            print(f"new distance is {new_distance}")

            distance = new_distance
            image    = new_image
            image.save(f"new{step+1}.png")
        else:
            print("failed to decrease distance, new state rejected")

main()