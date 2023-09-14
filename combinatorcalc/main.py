#!/usr/bin/env python3
import readline
from tree import *

class ParseError(Exception): pass

defined = {
    "S" : Combinator("S", lambda x, y, z : ((x, z), (y, z)) ),
    "K" : Combinator("K", lambda x, y    : x                ),
    "I" : Combinator("I", lambda x       : x                ),
    "B" : Combinator("B", lambda x, y, z : (x, (y, z))      ),
    "C" : Combinator("C", lambda x, y, z : ((x, z), y)      ),
    "W" : Combinator("W", lambda x, y    : ((x, y), y)      ),
}

def parse_tree(line):
    if len(line) == 1:
        if line in defined:
            return defined[line]
        return Symbol(line)

    segs = []
    start = None
    indent = 0

    for i, c in enumerate(line):
        if c == "(":
            if indent == 0:
                start = i+1
            indent += 1
        elif c == ")":
            indent -= 1
            if indent < 0:
                raise ParseError("Mismatched brackets")
            if indent == 0:
                segs.append(line[start:i])
        elif indent == 0:
            segs.append(c)

    if indent > 0:
        raise ParseError("Mismatched brackets")

    segs = [parse_tree(seg) for seg in segs]

    def fold_tree(s):
        if len(s) == 1:
            return s[0]
        rhs = s[-1]
        lhs = fold_tree(s[:-1])
        return Tree(lhs, rhs)

    return fold_tree(segs)

def do_reduction_loop(line):
    tree = parse_tree(line)
    intermediate, final = full_reduce_tree(tree)
    for value in intermediate:
        print(value.show())

def do_assignment(line):
    split = line.split("=")
    if len(split) > 2:
        raise ParseError("Stray '=' in expression")

    lhs, rhs = split
    if len(lhs) != 1:
        raise ParseError("Invalid left hand of assignment")

    tree = parse_tree(rhs)
    _, tree = full_reduce_tree(tree)
    defined[lhs] = tree
    print(f"{lhs}={tree.show()}")

def get_expression():
    try:
        line = input("> ").replace(" ", "")
        if line == "":
            return True
        if "=" in line:
            do_assignment(line)
        else:
            do_reduction_loop(line)
        return True
    except EOFError: # ugh
        print("CTRL + D")
        return False
    except KeyboardInterrupt:
        print("CTRL + C")
        return False
    except ParseError as err:
        print(f"Error while parsing: {err}")
        return True

def main():
    running = True
    while running:
        running = get_expression()

main()
