#!/usr/bin/env python3
from pyautogui import typewrite
from time import sleep
from sys import argv

sleepfor = 3

if len(argv) == 2:
    filename = argv[1]
    with open(filename, "r") as f:
        contents = f.read()
    print(f"Waiting {sleepfor} seconds to execute...")
    sleep(sleepfor)
    typewrite(contents)
else:
    print(f"Usage: {argv[0]} [file]")