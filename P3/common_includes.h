/***********************************************************************************
 * Unix Network Programming - Progarmming Assignment 3
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
/*For Semaphore*/
#include <semaphore.h>

#define SERVER_FILE_PATH "./Server"
#define POSIX_SHM_NAME "ipc" // File mapped to memory
#define POSIX_SEM_WAIT "/sem_vpandey_wait"
#define SYSV_SHM_NAME "/tmp"
#define MAX_BUFFER_SIZE 500 

#define ZERO(buff, size) memset (buff, 0, size);
#define STR_GETOPT "i:"

#define BASE10 10

#define USAGE_EXIT(process)\
                                        printf ("\t Illegal Usage. Expected option <-i>. Correct usage for the this utility is:\n"); \
                                        printf ("\t %s -i <ipc_name>\n", process); \
                                        printf ("\t Following are the expected IPC name. Please Note that these are case sensitive.\n"); \
                                        printf ("\t 1. POSIX_SHM\n"); \
                                        printf ("\t 2. SYSTEMV_SHM\n"); \
                                        printf ("\t \t \t @IIST utility...\n"); \
                                        exit(-1);

#define PRINT_USAGE \
                                        printf ("\t There are five basic commands to this utility: \n"); \
                                        printf ("\t 1. LS \t\t\t- lists all the files and directories in the current folder\n"); \
                                        printf ("\t 2. READ <file_path> \t- trigger a read fop operation on server. The result will be given by ipc_utility's stdout\n"); \
                                        printf ("\t 3. DELETE <file_path> \t- trigger a delete fop operation on server and file will be deleted from server\n"); \
                                        printf ("\t 4. CLEAR \t\t- clear the terminal screen\n"); \
                                        printf ("\t 5. HELP \t\t- print this command information\n"); \
                                        printf ("\t 6. EXIT \t\t- shutdown the server gracefully and cleanup. Always recommended instead of CTRL+C\n"); \
                                        printf ("\t \t \t @IIST utility...\n");


#define BAD_USAGE(process, token) \
                                        printf ("\nBad command or file name\n"); \
                                        printf ("%s is unabe to recognize input token \"%s\"\n", process, token); \
					printf ("The correct usage of the File server is as followes -\n"); \
					PRINT_USAGE

#define BASE10 10

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
        ipc_mode_posix_shm,
        ipc_mode_systemv_shm
} ipc_mode_t;

typedef union { //Not really needed. Kept for readability
        char*   systemv_shm;
        char*   posix_shm;
	sem_t*	semaphore;
} ipc_t;


#endif
