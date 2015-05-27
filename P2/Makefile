####################################################################################
 # Unix Network Programming - Progarmming Assignment 2
 # File name 	: Make file
 # Module 	: Make
 # Description	: This file contains the make information associated with the project. 
####################################################################################
Ct=gcc
CCFlags=-lrt

all: ipc_utility.out

debug: CCFlags += -DDEBUG -g
debug: ipc_utility.out

ipc_utility.out: ipc_utility.c
	$(CC) $(CCFlags) ipc_utility.c -o ipc_utility

clean:
	rm -rvf *.o *.out core* ipc ipc_utility
