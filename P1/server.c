/***********************************************************************************
 * Unix Network Programming - Progarmming Assignment 1
 * File name 	: server.c
 * Module 	: Server.out
 * Description	: This file basically handles all the server operation requested from 
		  the client. Server will have minimum o/p interaction (unless fatal).
		  The output will be passed on to the client where it will be 
		  displayed to the user.
***********************************************************************************/

#include "common_includes.h"

volatile bool trigger_signal = false;
struct sigaction old_action, new_action;

void signal_handler (int signum);
int get_line(char *buffer, size_t max);

int main (int argc, const char* argv[])
{
	struct stat sb;
	int ret_val = 0;
	sigset_t zero_sigset;
	FILE* file = NULL, *file_to_process = NULL;
        char file_path[PATH_MAX];

	pid_t pid_client = getppid();
	char* buffer =  (char*) malloc (MAX_BUFFER_SIZE);
	
	//initialize new action
	sigemptyset (&new_action.sa_mask);
	new_action.sa_flags = 0;
	new_action.sa_handler = signal_handler; 	// function pointer to the SIGUSR1 handler
	sigaddset (&new_action.sa_mask, SIGUSR1);

	//initialize old action
	sigemptyset (&old_action.sa_mask);

	//initialize zero mask
	sigemptyset (&zero_sigset);

	/*
 	 * Set a handler for SIGUSR1
 	 * This will later be used to set the flag and run the signal routine
	 */
	if (ret_val = sigaction (SIGUSR1, &new_action, &old_action) != 0)
	{
		printf ("\nFatal Error! Unable to install the signal on Server.\nTerminating now...");
		return ret_val;
	}
	
	for (;ret_val == 0;){
		
        	ZERO (file_path, PATH_MAX);
		ZERO (buffer, MAX_BUFFER_SIZE);	
		
		while (!trigger_signal) 
			sigsuspend (&zero_sigset);

		trigger_signal = false;
		
		//Server recvd SIGUSR1
		//Open IPC file
		command user_command = command_not_set;
		file = fopen (IPC_FILE_PATH, "r");
		if (file == NULL)
		{
			printf (buffer, "Fatal Error! Server is unable to open IPC file.\nTerminating now...");
			break;
		}
		
		//Get the command request from the file
		if (!fscanf (file, "%d:%s", &user_command, file_path))
			user_command = command_bad;

		//close the file. Reopen it in w mode
		if (file)
		{
			fclose (file); //Reopen in w mode to refresh its content
			file = NULL;
		}
	
		//write the o/p to the file
		file = fopen (IPC_FILE_PATH, "w");
		
		//Check if the path is valid.
		if (-1 == stat (file_path, &sb)){
			//file does not exits;
			fprintf	(file, "File path \"%s\" does not exit.\nPlease check the file path and try again.", file_path);
		}
		else {
			// Do the requested file op
			switch (user_command)
			{
				case command_read:
					file_to_process = fopen (file_path, "r");
					if (file_to_process == NULL)
						sprintf	(buffer, "We have encountered some error while reading the file - %s.\nPlease make sure that this file is readable by current user", file_path);
					else {
						while (fgets (buffer, MAX_BUFFER_SIZE, file_to_process)) //Read the file and write the o/p to the ipc
						{
							fprintf (file, "%s", buffer);
							ZERO (buffer, MAX_BUFFER_SIZE);	
						}
					}
					break;
				case command_delete:
					if (0 == remove (file_path)) //Perform the file operation and write the result in a file
						fprintf (file, "File deleted!!!");
					else
						fprintf (file, "Unable to delete the file!!!");

					break;
				case command_bad:
					fprintf (file, "Bad command or file name", user_command); //LOL. I am trying to be nostalgic here
					break;
				default:
					break;
			}

			if (file_to_process) // Close the file to process and set fd to NULL
			{
				fclose (file_to_process);
				file_to_process = NULL;
			}
		}

		if (file) // This will ensure that if there is no response from the server, atleast we will refresh the ipc file
		{
			fclose (file);
			file = NULL;
		}

		//Signal Client that there is a response...
		if (-1 == kill (pid_client, SIGUSR1))
		{
			printf ("\nFatal Error! Unable to reach the server. \nReason: %s\nTerminatinr now...", strerror(errno)); 
			ret_val = errno;
			continue;	
		}
	}
	
	sigprocmask (SIG_SETMASK, &new_action.sa_mask, &old_action.sa_mask); // __FixMe__ is it really necessary. I am anyways dying... 
	if (buffer)
		free (buffer);

	return 0;
}

int get_line(char *buffer, size_t max)
{
  	if (fgets (buffer, max, stdin) == buffer)
  	{
		size_t len = strlen(buffer);
   		if (len > 0 && buffer[len - 1] == '\0')
      			buffer[--len] = '\0';
    		return len;
  	}

  	return 0;
}

void signal_handler (int signum)
{
	trigger_signal = true;
	sigprocmask (SIG_BLOCK, &new_action.sa_mask, &old_action.sa_mask); // Always a good idea, because kernel is God, can't be per-empt
}
