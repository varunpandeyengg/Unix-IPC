####################################################################################
 # Unix Network Programming - Progarmming Assignment 3
 # File name 	: Make file
 # Module 	: Make
 # Description	: This file contains the make information associated with the project. 
####################################################################################
Ct=gcc
CCFlags=-lrt

all: Server.out Client.out

debug: CCFlags += -DDEBUG -g
debug: Server.out Client.out

Server.out: server.c
	$(CC) $(CCFlags) server.c -o Server

Client.out: client.c
	$(CC) $(CCFlags) client.c -o Client

clean:
	rm -rvf *.o *.out core* ipc Client Server
