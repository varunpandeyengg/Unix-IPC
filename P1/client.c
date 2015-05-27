/***********************************************************************************
 * Unix Network Programming - Progarmming Assignment 1
 * File name 	: client.c
 * Module 	: Client.out
 * Description	: Client module is responsible of taking user's request and transfering 
		  it to the Server. Client also invokes the server. All communication 
		  are synced through SIGUSR1. If everything works alright, you will see
		  an internal commandline '$'. Press EXIT to terminame  
***********************************************************************************/
#include "common_includes.h"

volatile bool trigger_signal = false;
struct sigaction old_action, new_action;

void signal_handler (int signum);
int get_line(char *buffer, size_t max);

int main (int argc, const char* argv[])
{
	int ret_val = 0;
	FILE* file;
	pid_t pid_server;
	sigset_t zero_sigset;	
	char* buffer =  (char*) malloc (MAX_BUFFER_SIZE);
	
	//Invoke Server
	printf ("\nWelcome to a PIST - Primitive IPC Simulation Tool!\nCloning Client...\n");	
	
	pid_server = fork ();
	if (pid_server == 0)
	{
		printf ("\nCloning Successful!\nSpawning Server to process with PID %d\n", getpid());
		
		execlp (SERVER_FILE_PATH, NULL); //No arguments passed to the server

		ret_val = errno;

		printf ("\nFatal Error! Unable to start a server. Reason: %s\nTerminatinr now... ", strerror(errno)); // If exec returns, it is definitely an error
		kill (getppid(), 9);
		return ret_val;
	}
	
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
		printf ("\nFatal Error! Unable to install the signal.\nTerminatinr now...");
		return ret_val;
	}
	
	sleep (1); // give chance to "UP" the server
	printf ("\nThe File Server is up now. Enter commands to access the server.\n");
	
	PRINT_USAGE;
	
	for (;ret_val == 0;){
		printf ("$ ");
		ZERO (buffer, MAX_BUFFER_SIZE);	
	
		if (fgets (buffer, MAX_BUFFER_SIZE, stdin)) // Limited size command. Assuming that file path won't excceed PATH_MAX charecters
		{
			command user_command = command_not_set;
			char* token = strtok (buffer, " \n"); //breaking the incoming commands from user with boundary as ' ' or '\n'

   			//printf ("Token %s", token); //For debug puropose
			if (!token) // If user command has initial white spaces, this may trigger
				continue; //Trim and skip such tokens 

			if (0 == strcmp (token, "READ"))
				user_command = command_read;
			else if (0 == strcmp (token, "DELETE"))
				user_command = command_delete;
			else if (0 == strcmp (token, "EXIT"))
				user_command = command_exit;
			else if (0 == strcmp (token, "CLEAR"))
			{
				system("clear");
				continue;
			}
			else if (0 == strcmp (token, "HELP"))
			{
				PRINT_USAGE;
				continue;
			}
			
			if (user_command == command_not_set)
			{
				BAD_USAGE (argv[0], token);
				continue;
			}
			
			//
			if (user_command == command_exit) 
			{
				// Cleanup, kill Server and die
				remove (IPC_FILE_PATH);
				printf ("\nTerminating now... \n");
                		kill (pid_server, 9);
                		return ret_val;
			}
			
			token = strtok (NULL, " \n");			
			if (!token)
			{
				BAD_USAGE (argv[0], token);
				continue;
			}

   			//printf ("Token %s", token); //For debug purpose
			FILE* file = fopen (IPC_FILE_PATH, "w");
			if (file)
			{
				fprintf (file, "%d:%s", user_command, token);// Protocol used for server and client to communicate. <Command_number>:<file_path>
				
				//Close the file for further use
				fclose (file);
				file = NULL;
				
				//Signal Server that there is a request...
				if (-1 == kill (pid_server, SIGUSR1))			
				{
					printf ("\nFatal Error! Unable to reach the server. \nReason: %s\nTerminatinr now...", strerror(errno)); 
					ret_val = errno;
				}
			}
		}

		while (!trigger_signal) 
			sigsuspend (&zero_sigset);

		trigger_signal = false;		
		printf (" Response received from Server:\n");
		
		file = fopen (IPC_FILE_PATH, "r");
		if (file != NULL)
		{
 			while (fgets (buffer, MAX_BUFFER_SIZE, file))
                	{
                		printf ("%s", buffer);
                        	ZERO (buffer, MAX_BUFFER_SIZE);
			}
			
			fclose (file);
			file = NULL;
		}
		printf ("\n");
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

	sigprocmask (SIG_BLOCK, &new_action.sa_mask, &old_action.sa_mask);
}
