/***********************************************************************************
 * Unix Network Programming - Progarmming Assignment 1
 * File name 	: common_includes.h
 * Module 	: Client.out and Server.out
 * Description	: This file basically handles all the common headers and include files 
		  that are needed for above mentioned modules.
***********************************************************************************/

#ifndef COMMON_INCLUDES
#define COMMON_INCLUDES
#include <stdio.h>

#include <signal.h>

/*For error information*/
#include <error.h>
#include <errno.h>

#include <stdbool.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/*To check if the file is present or not*/
#include <sys/stat.h>

/*for PATH_MAX*/
#include <linux/limits.h>

#define IPC_FILE_PATH "ipc"
#define SERVER_FILE_PATH "./Server"

#define MAX_BUFFER_SIZE PATH_MAX*2 

#define ZERO(buff, size) memset (buff, 0, size);
#define PRINT_USAGE \
					printf ("\t There are five basic commands to this utility: \n"); \
					printf ("\t 1. READ <file_path> - trigger a read fop operation on server. The result will be given by client's stdout\n"); \
					printf ("\t 2. DELETE <file_path> - trigger a delete fop operation on server and file will be deleted from server\n"); \
					printf ("\t 3. EXIT - shutdown the server gracefully and cleanup. Always recommended instead of CTRL+C\n"); \
					printf ("\t 4. CLEAR - clear the terminal screen\n"); \
					printf ("\t 5. HELP - print this command information\n"); \
					printf ("\t \t \t @PIST utility...\n");

#define BAD_USAGE(process, token) \
                                        printf ("\nBad command or file name\n"); \
                                        printf ("%s is unabe to recognize input token \"%s\"\n", process, token); \
					printf ("The correct usage of the File server is as followes -\n"); \
					PRINT_USAGE
typedef enum _command
{
        command_not_set,
        command_read,
        command_delete,
        command_exit,
	command_bad
} command;

#endif
