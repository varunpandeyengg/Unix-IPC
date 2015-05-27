####################################################################################
 # Unix Network Programming - Progarmming Assignment 1
 # File name 	: Make file
 # Module 	: Make
 # Description	: This file contains the make information associated with the project. 
####################################################################################
CC=gcc
CCFlags= 

all: Server.out Client.out

debug: CCFlags += -DDEBUG -g
debug:  Server.out Client.out

Server.out: server.c
	$(CC) $(CCFlags) server.c -o Server

Client.out: client.c
	$(CC) $(CCFlags) client.c -o Client

clean:
	rm -rf *.o *.out core* ipc Server Client
