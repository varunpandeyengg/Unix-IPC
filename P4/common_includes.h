/***********************************************************************************
 * Unix Network Programming - Progarmming Assignment 4
 * File name 	: common_includes.h
 * Module 	: Client.out and Server.out
 * Description	: This file basically handles all the common headers and include files 
		  that are needed for above mentioned modules.
***********************************************************************************/

#ifndef COMMON_INCLUDES
#define COMMON_INCLUDES
#include <stdio.h>

/*For error information*/
#include <error.h>
#include <errno.h>

#include <stdbool.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/*To check if the file is present or not*/
#include <sys/stat.h>

/*For System V SHM*/
#include <sys/shm.h>

/*For O_RDWR*/
#include <fcntl.h>

/*For PROT_WRITE*/
#include <sys/mman.h>

/*for PATH_MAX*/
#include <linux/limits.h>
/*For Sockets*/
#include <sys/socket.h>
/*For sockaddr_in*/
#include <netinet/in.h>

/*For addrinfo*/
#include <netdb.h>
#include <ifaddrs.h>

#define SERVER_FILE_PATH "./Server"
#define POSIX_SEM_NAME "/sem_vpandey" //For signaling that the server is up

#define MAX_BUFFER_SIZE 500 

#define ZERO(buff, size) memset (buff, 0, size);
#define STR_GETOPT "i:h:p:"

#define BASE10 10

#define USAGE_EXIT(process)\
                                        printf ("\t Illegal Usage. Expected option <-i>. Correct usage for the this utility is:\n"); \
                                        printf ("\t %s -i <ipc_name> ", process); \
					if (strcasestr (process, "client")) \
						printf("-h <host name> -p <port#>\n"); \
					else \
						printf ("\n"); \
                                        printf ("\t Following are the expected IPC name. Please Note that these are case sensitive.\n"); \
                                        printf ("\t 1. UDP\n"); \
                                        printf ("\t 2. TCP\n"); \
                                        printf ("\t \t \t @SIST utility...\n"); \
                                        exit(-1);

#define PRINT_USAGE \
                                        printf ("\t There are five basic commands to this utility: \n"); \
                                        printf ("\t 1. LS \t\t\t- lists all the files and directories in the current folder\n"); \
                                        printf ("\t 2. READ <file_path> \t- trigger a read fop operation on server. The result will be given by ipc_utility's stdout\n"); \
                                        printf ("\t 3. DELETE <file_path> \t- trigger a delete fop operation on server and file will be deleted from server\n"); \
                                        printf ("\t 4. CLEAR \t\t- clear the terminal screen\n"); \
                                        printf ("\t 5. HELP \t\t- print this command information\n"); \
                                        printf ("\t 6. EXIT \t\t- shutdown the server gracefully and cleanup. Always recommended instead of CTRL+C\n"); \
                                        printf ("\t \t \t @SIST utility...\n");


#define BAD_USAGE(process, token) \
                                        printf ("\nBad command or file name\n"); \
                                        printf ("%s is unabe to recognize input token \"%s\"\n", process, token); \
					printf ("The correct usage of the File server is as followes -\n"); \
					PRINT_USAGE

#define CONNECTION_DETAILS(ip_addr, port_no) \
					printf ("\n______________Connection Details_______________");	\
					printf ("\nServer is located at the IP address: %s \n and has been allocated a Server port: %d\n_______________________________________________\n", ip_addr, port_no);

#define BASE10 10
#define BACKLOG 10
#define CHECK(x) \
    	{ \
		do { \
        		fprintf(stderr, "%s:%d: ", __func__, __LINE__); \
            		perror(#x); \
            		exit(-1); \
        	} while (0); \
	}

#define COMMAND_LS "ls -a"

#define FILE_MODE S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
#define IPC_OPEN_ERR "\nFatal Error! Unable to create ipc channel. Reason: %s\nTerminatinr now... "
/*All the commands supported IIST*/
typedef enum
{
        ipc_command_not_set,
        ipc_command_read,
        ipc_command_delete,
        ipc_command_exit,
        ipc_command_clear,
        ipc_command_help,
        ipc_command_ls,
        ipc_command_bad
} ipc_command;

typedef enum {
        ipc_mode_not_set,
        ipc_mode_udp_sock,
        ipc_mode_tcp_sock
} ipc_mode_t;

typedef union { //Not really needed. Kept for readability
        int 	tcp_fd;
        int   	udp_fd;
} ipc_t;


#endif
