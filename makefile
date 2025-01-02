#Shahaddin Gafarov
#I have simplified the make file I have used, In my previous implemention I have made the command: if [ -f rush ]; then echo SUCCESS; fi builtin
#Basically what my makefile does is upon typing make, compiles the gcc and gives an executible rush.o(gcc -Wall -Werror -c rush.c)

#defining the compiler to be used
CC = gcc
#the flags of the compiler
CFLAGS = -Wall -Werror

#our target is to the rush
all: rush

#for getting rush as executable, we need rush.o first
rush: rush.o
#this line turns the object file into executable using -o command
	@$(CC) rush.o -o rush

#for getting rush.o, we need rush.c first
rush.o: rush.c
#this is the command to compiler
#-c is for creating object file(.0) instead of an executable
	@$(CC) $(CFLAGS) -c rush.c

#default cleanup operation
clean:
	rm -f rush.o

#I used this as a source for building my makefile
#Sources:https://makefiletutorial.com/