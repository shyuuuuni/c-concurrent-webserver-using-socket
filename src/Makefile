# @file   Makefile
# @usage	$ make : Make Executable
# 				$ ./server {port number} : Execute web server with your port number
#					$ make clean : Clear object files and Executable
# author	Seunghyun Kim
CC=gcc
CFLAGS=-g -Wall
OBJS=server.o
TARGET=server

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS)

server.o: server.c
	gcc -c server.c

clean:
	rm -f *.o
	rm -f *.out
	rm -f $(TARGET)