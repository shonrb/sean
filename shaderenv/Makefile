SRC = $(wildcard src/*.cpp)
LIB = -lSDL2 -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lGLEW
FLAGS = -std=c++2a -Wall -Wextra -Wpedantic
PARAMS = $(LIB) $(FLAGS)

default:
	g++ $(SRC) $(PARAMS) -o prog