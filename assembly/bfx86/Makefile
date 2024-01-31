default:
	nasm bf.s -felf64 -o bf.o
	ld bf.o -o prog
	rm bf.o
info:
	gcc info.c -o infoprog
	./infoprog
	rm infoprog