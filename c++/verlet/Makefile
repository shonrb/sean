SRC = $(wildcard *.cpp) external/glad/src/glad.c
LIBS = -lsfml-system -lsfml-window -lsfml-graphics -lGL -lX11 -lpthread -lXrandr -lXi -ldl
EXT = -Iexternal/glad/include
FLAGS = -std=c++2a -fconcepts

default:
	g++ $(SRC) $(LIBS) $(EXT) $(FLAGS) -o prog