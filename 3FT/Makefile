#--------------------------------------------------------------------
# Makefile for Assignment 4, Part 3
# Author: Eesha and Maria
#--------------------------------------------------------------------

CC = gcc217
#CC = gcc217m

ft: dynarray.o path.o checkerFT.o nodeFT.o ft.o ft_client.o
	$(CC) -g dynarray.o path.o checkerFT.o nodeFT.o ft.o ft_client.o -o ft

dynarray.o: dynarray.c dynarray.h
	$(CC) -g -c dynarray.c

path.o: path.c path.h dynarray.h a4def.h
	$(CC) -g -c path.c

checkerFT.o: checkerFT.c checkerFT.h nodeFT.h path.h dynarray.h a4def.h
	$(CC) -g -c checkerFT.c

nodeFT.o: nodeFT.c nodeFT.h checkerFT.h path.h dynarray.h a4def.h
	$(CC) -g -c nodeFT.c

ft.o: ft.c ft.h nodeFT.h checkerFT.h path.h dynarray.h a4def.h
	$(CC) -g -c ft.c

ft_client.o: ft_client.c ft.h a4def.h
	$(CC) -g -c ft_client.c

clean:
	rm -f ft dynarray.o path.o checkerFT.o nodeFT.o ft.o ft_client.o meminfo*.out