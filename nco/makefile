SRC = $(wildcard src/*.v) $(wildcard src/*.cpp)
FLAGS = -Wall --cc --exe --build -GOUT_SIZE=16 -GACC_SIZE=24

default: $(SRC)
	verilator $(FLAGS) $(SRC)

