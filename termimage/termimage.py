''' termimage.py
    Module to print an image to ANSI terminals.
'''

import json 
from PIL           import Image, ImageOps
from math          import sqrt, inf
from functools     import lru_cache
from scipy.spatial import KDTree

def _get_scaled_size(scale: int, image_size: tuple) -> tuple:
    ''' Scales the size of an image to be of height 'scale' in terminal
        cells. '''
    width, height = image_size
    aspect_ratio = width / height

    # Width is doubled as most terminals have a 1:2 character size
    return (int(scale * aspect_ratio * 2), scale) 

def _greyscale(colour: tuple) -> int:
    ''' Convert a colour to it's greyscale equivalent. '''
    
    r, g, b = colour

    # Values based on the eye's sensitivity to light
    grey = int(0.299 * r + 0.587 * g + 0.114 * b)
    return grey


def _red_mean_difference(colour0: tuple, colour1: tuple) -> float:
    ''' Calculates the difference between two colours.

        Unlike euclidean distance, this method accounts for
        the human eye's varying sensitivity to different light
        colours.

        https://en.wikipedia.org/wiki/Color_difference 
    '''
    
    dr, dg, db = (c0 - c1 for c0, c1 in zip(colour0, colour1))

    redmean = (colour0[0] + colour1[0]) / 2

    R = dr**2 * (2 + redmean / 256)
    G = dg**2 + 4
    B = db**2 * (2 + (255 - redmean) / 256)

    return (R + G + B)

@lru_cache(maxsize = None)
def _closest_in_tree(tree: KDTree, colour: tuple) -> int:
    ''' Finds the nearest neighbour of a colour in a KD tree.

        This method trades the accuracy of the red mean method
        for a large increase in performance. For use with large 
        lookup tables
     '''
    _, index = tree.query(colour)
    return index

def _closest_in_table(table: dict, colour: tuple) -> tuple:
    ''' Searches a dictionary with colour tuples as keys and returns
        the value of the key which best matches the input colour 
    '''
    
    selected_colour = None
    selected_diff = inf
    
    for table_colour in table.keys():
        diff = _red_mean_difference(colour, table_colour)
        
        if diff < selected_diff:
            selected_colour = table_colour
            selected_diff = diff

    return table[selected_colour]

@lru_cache(maxsize=None)
def _convert_image(char_data: tuple) -> str:
    ''' Converts a 2d array of characters to a string '''

    return "\n".join(["".join(row) + "\033[0m" for row in char_data])

@lru_cache(maxsize=None)
def _term_image_ascii(data: tuple) -> str:
    ''' Converts the image data to regular ascii characters. '''
    table = " .:|+=SN$#@"

    def make_char_row(row: tuple) -> tuple:
        for colour in row:
            value = _greyscale(colour)
        
            index = int(value / 255 * (len(table)-1))
            yield table[index]
    
    chars = tuple(make_char_row(row) for row in data)

    return _convert_image(chars)

@lru_cache(maxsize=None)
def _term_image_16(data: tuple) -> str:
    ''' Converts the image data to escaped characters with
        16-colour escape codes.
    '''

    colour_table = {
        (0,   0,   0)   : 40,
        (255, 0,   0)   : 41,
        (0,   255, 0)   : 42,
        (255, 255, 0)   : 43,
        (0,   0,   255) : 44,
        (255, 0,   255) : 45,
        (0,   255, 255) : 46,
        (255, 255, 255) : 47,
    }

    # Function to generate each row
    def make_char_row(row: tuple) -> tuple:
        for colour in row:
            code = _closest_in_table(colour_table, colour)

            yield f"\033[;{code}m "

    chars = tuple(make_char_row(row) for row in data)

    return _convert_image(chars)

@lru_cache(maxsize=None)
def _term_image_256(data: tuple) -> str:
    ''' Converts the image data to escaped characters with
        256-colour escape codes.
    '''

    # Data from https://jonasjacek.github.io/colors/data.json
    with open('256.json', 'r') as f:
        colours256 = json.load(f)

    # Convert the RGB data to a 3D array
    rgbs256 = [(t['rgb']['r'], t['rgb']['g'], t['rgb']['b']) 
              for t in colours256]

    # Then to a KD tree
    tree256 = KDTree(rgbs256)

    # Function to generate each row
    def make_char_row(row: tuple) -> tuple:
        for colour in row:
            index = _closest_in_tree(tree256, colour)
            code = colours256[index]["id"]

            yield f"\033[48;5;{code}m "

    chars = tuple(make_char_row(row) for row in data)

    return _convert_image(chars)

@lru_cache(maxsize=None)
def _term_image_rgb(data: tuple) -> str:
    ''' Converts the image data to escaped characters with
        RGB escape codes. 
    '''

    rows = []

    for col in data:
        row = tuple(f"\033[48;2;{r};{g};{b}m " for r, g, b in col)
        rows.append(row)

    return _convert_image(tuple(rows))

def _term_image_grey(data: tuple) -> str:
    pass

def image_as_string(path: str, size: tuple or int, output: str) -> str:
    ''' Takes an image path and converts the image there 
        to a string.
        
        size can either be a tuple: (width, height), or an int containing 
        the scale while keeping the proportions of the original image).
    '''
    
    # Determine the image type to create from 'output'
    outputs = {
        "ascii": _term_image_ascii,
        "16"   : _term_image_16,
        "256"  : _term_image_256,
        "rgb"  : _term_image_rgb,
        "grey" : _term_image_grey
    }
    try:
        make_image = outputs[output]
    except:
        return f"Invalid option: {output}"
    
    image = Image.open(path)

    # If size is given as a scale, convert to a tuple
    if type(size) == int:
        size = _get_scaled_size(size, image.size)

    # Resize the image, then get the raw colour data 
    image = image.resize(size, resample=Image.LANCZOS).convert("RGB")
    data = list(image.getdata())
    width, height = size

    # Convert the raw data to a 2D array
    # Array goes [y][x] to ease printing to the terminal
    data = tuple(tuple(
        data[y * width + x] for x in range(width)
    ) for y in range(height))

    return make_image(data)


def print_image(path: str, size: tuple or int, output: str) -> str:
    ''' Prints an image from an image path. '''
    
    print(image_as_string(path, size, output))


if __name__ == "__main__":
    import sys
    import os
    import time

    if len(sys.argv) != 3:
        print("usage: [python] termimage.py [file] [format]")
        print("Formats: rgb, 256, 16, grey, ascii")
    else:
        path  = os.path.join(os.path.dirname(__file__), "") + sys.argv[1]
        print_image(path, 20, sys.argv[2])
