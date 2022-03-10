#!/usr/bin/env python3
''' sloc.py
    SLOC counter for several languages.
    A SLOC is a line of code which if removed
    would alter the program's functioning.
'''
import json
from sys import argv
from os import path, scandir

with open("comments.json") as f:
    COMMENTS_BY_LANGUAGE = json.load(f)

class CommentChecker:

    def __init__(self, syntax):
        self.level = 0
        self.current_block = None
        self.syntax = syntax

    @staticmethod
    def after(line, token, index):
        # Returns everything in line after token at index
        return line[index + len(token):]

    def handle_nestable_block(self, stripped):
        closing = self.current_block["close"]
        opening = self.current_block["open"]

        commented = True
        current = stripped

        while True:
            size = len(current)
            o = current.find(opening)
            c = current.find(closing)

            # If the tokens are not found, set them to the
            # size of the current line. This simplifies some
            # comparisons 
            o = o if o != -1 else size
            c = c if c != -1 else size

            if (o == size and c == size):
                # no more comments left, no further changes
                break
            
            if o < c:
                # Opening comes before closing
                self.level += 1
                current = self.after(current, opening, o)
            elif c < o:
                self.level -= 1

                # If the comment is ended, and a new one is not
                # immediately opened,
                opening_next = c + len(closing) == o

                if self.level == 0 and not opening_next:
                    commented = False
                current = self.after(current, closing, c)

        return commented

    def handle_non_nestable_block(self, stripped):
        # Try to find the closing syntax
        closing = self.current_block["close"]
        i = stripped.find(closing)

        if i == -1:
            # Not found
            return True
        
        # Found, leave the comment
        self.level = 0
        last_possible = len(stripped) - len(closing)
        
        # If the comment is the last thing on the 
        # line, the line is fully commented
        return i == last_possible

    def handle_inside_block(self, stripped):
        nestable = self.current_block["nestable"]
        if nestable:
            return self.handle_nestable_block(stripped)
        else:
            return self.handle_non_nestable_block(stripped)

    def handle_outside_block(self, stripped):
        comments_in_line = {}

        # Add each comment type present to 
        # comments_in_line, with the key being 
        # the index
        for comment in self.syntax:
            opening = comment["open"]
            i = stripped.find(opening)
            if i != -1:
                comments_in_line[i] = comment
        
        # If none were found the line is not commented
        if len(comments_in_line) == 0:
            return False
        
        # Use the earliest occurring one
        i = min(comments_in_line.keys())
        comment = comments_in_line[i]

        t = comment["type"]

        if t == "inline" and i == 0:
            # Block comment at beginning of line,
            # line is fully commented
            return True
        elif t == "block":
            commented = False

            # If nothing is before the comment, the
            # line is commented
            if i == 0:
                commented = True
            
            # We are now in a block comment
            self.current_block = comment
            self.level = 1

            # If there is nothing before, and the comment ends on
            # the same line and has nothing after, the line is fully
            # commented 
            opening = comment["open"]
            rest_of_line = self.after(stripped, opening, i)

            return commented and self.handle_inside_block(rest_of_line)
        return False

    def is_commented(self, line):
        stripped = line.strip()

        if self.level > 0:
            return self.handle_inside_block(stripped)
        else:
            return self.handle_outside_block(stripped)


def sloc_in_file(file, syntax):
    sloc = 0

    lines = file.readlines()
    checker = CommentChecker(syntax)

    for line in lines:
        not_empty = not line.isspace()
        not_commented = not checker.is_commented(line)

        if not_commented and not_empty:
            sloc += 1

    return sloc


def sloc_in_file_or_dir(name):
    if path.isdir(name):
        # Get the sloc for every file in the dir
        return sum(
            sloc_in_file_or_dir(f"{name}/{i.name}")
            for i in scandir(name)
        )

    # Ignore if there is no file extension
    split = name.split(".")
    if len(split) < 2:
        return 0

    # Ignore if the file extension is not recognised
    extension = split[-1]
    if extension not in COMMENTS_BY_LANGUAGE.keys():
        return 0

    # Get the comment syntax
    comment_syntax = COMMENTS_BY_LANGUAGE[extension]

    # Get the sloc from the file
    with open(name) as f:
        sloc = sloc_in_file(f, comment_syntax)
        print(f"{name}: {sloc} SLOC")
        return sloc


def main():
    if len(argv) != 2:
        print(f"Usage: {argv[0]} [target]")
        return
    
    name = argv[1]
    if not path.exists(name):
        print(f"{target}: No such file or directory")
        return

    sloc = sloc_in_file_or_dir(name)
    print(f"TOTAL: {sloc} SLOC")

if __name__ == "__main__":
    main()