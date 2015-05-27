/***********************************************************************************
 * Unix Network Programming - Progarmming Assignment 2
 * File name 	: ipc_utility.c
 * Module 	: ipc_utility.out
 * Description	: Client module is responsible of taking user's request and transfering 
		  it to the Server. Client also invokes the server. This module will 
		  implement 4 IPC mechanisms - pipes, fifo, posix shared memory and 
		  system V shared memory. If everything works alright, you will see
		  an internal commandline '$'. Press EXIT to terminate
***********************************************************************************/
#include "common_includes.h"

Blob* blob;
const char *program_name;
ipc_mode_t ipc_mode = ipc_mode_not_set; //for now

void display_result ();
bool create_ipc (ipc_t* ipc_type);
void open_ipc (ipc_t* ipc_type, ipc_open_mode_t mode, const char* ipc_path); //mode could be ignored. Depends upon the ipc type
void close_ipc (ipc_t* ipc_type, ipc_open_mode_t mode, const char* ipc_path); //mode could be ignored. Depends upon the ipc type

int get_line (char *buffer, size_t max);
void recv_data (ipc_t ipc_to_client);
void interpret_command (char* token, ipc_command *user_command);
void process_file (ipc_command user_command, char* file_path, char* buffer);
void send_data (ipc_t ipc_to_server);
void server (ipc_mode_t ipc_mode, ipc_t ipc_to_server, ipc_t ipc_to_client);
void client (ipc_mode_t ipc_mode, ipc_t ipc_to_server, ipc_t ipc_to_client, pid_t pid_server);

int main (int argc, char * const  argv[])
{
	int ret_val = 0;
	program_name = argv[0];

	//pipe is between related process
	//so sadly, I had to write a specialized code that basically 
	//creates new unmaned pipes. I should be used as a wormhole 
	//experement for doing this. What if... Never mind
	ipc_t ipc_to_server, ipc_to_client;

	char arg;
        while ((arg = getopt (argc, argv, STR_GETOPT)) != -1) {
                switch(arg)
                {
                        case 'i':
				if (0 == strcmp (optarg, "PIPE"))
					ipc_mode = ipc_mode_pipe;
				else if (0 == strcmp (optarg, "FIFO"))
					ipc_mode = ipc_mode_fifo;
				else if (0 == strcmp (optarg, "POSIX_MSG_Q"))
                                        ipc_mode = ipc_mode_posix_msg_q;
				else if (0 == strcmp (optarg, "SYSTEMV_MSG_Q"))
                                        ipc_mode = ipc_mode_systemV_msg_q;
                                break;
                        default: 		//redundant
				break;
                }
        }
	
	if (ipc_mode == ipc_mode_not_set)
	{
		USAGE_EXIT (argv[0]);
	}

	// Create the IPC mode used for communication between server and client	
	create_ipc (&ipc_to_server);
	create_ipc (&ipc_to_client);

	//Invoke Server
	printf ("\nWelcome to a IIST - Intermediate IPC Simulation Tool!\nCloning Client...\n");	

	pid_t pid_server = fork ();
	switch (pid_server)
	{
		case 0: //Server
		{
			//For unnamed pipe, here we close the other end of the pipe
			//A good practice is to close the end of pipe that is not going to be of use
			
			printf ("\nCloning Successful!\nSpawning Server to process with PID %d\n", getpid());
			open_ipc (&ipc_to_server, ipc_open_read, IPC_PATH_TO_SERVER); 
			if (ipc_mode == ipc_mode_posix_msg_q || ipc_mode == ipc_mode_systemV_msg_q)
				open_ipc (&ipc_to_client, ipc_open_write, IPC_PATH_TO_SERVER);
			else
				open_ipc (&ipc_to_client, ipc_open_write, IPC_PATH_TO_CLIENT);
		
			/*
			 * Following arguments passed to the server
			 * 1. IPC Mode - to 
			 * 2. ipc_to_server - will open for reading
			 * 3. ipc_to_client - will open for writing
			 */

			//execlp (SERVER_FILE_PATH, ipc_mode, ipc_to_server, ipc_to_client);
			
			server (ipc_mode, ipc_to_server, ipc_to_client);
			//ret_val = errno;

			//printf ("\nFatal Error! Unable to start a server. Reason: %s\nTerminatinr now... ", strerror(errno)); // If exec returns, it is definitely an error
			//kill (getppid(), 9);
			return ret_val;
		}
		case -1:
		{
        	        printf ("\nFatal Error! Unable to invoke server. Reason: %s\nTerminatinr now... ", strerror(errno));
                	ret_val = errno; //redundant, for consistancy
                	return ret_val;
	        }
		
		default: //Client
		{
			//For unnamed pipe, here we close the other end of the pipe
			//A good practice is to close the end of pipe that is not going to be of use
			//An easy trade off for for writing generic code is to have to desc for msg_q
			//Although we can manage with single msg queue 
			open_ipc (&ipc_to_server, ipc_open_write, IPC_PATH_TO_SERVER); //Write, reading closed
                        if (ipc_mode == ipc_mode_posix_msg_q || ipc_mode == ipc_mode_systemV_msg_q)
				open_ipc (&ipc_to_client, ipc_open_read, IPC_PATH_TO_SERVER); //Read, writing closed
			else
				open_ipc (&ipc_to_client, ipc_open_read, IPC_PATH_TO_CLIENT); //Read, writing closed

			client (ipc_mode, ipc_to_server, ipc_to_client, pid_server);
		}
	}

	
	if (blob)
		free (blob);

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

void server (ipc_mode_t ipc_mode, ipc_t ipc_to_server, ipc_t ipc_to_client)
{
	bool close_server = false;	
	
	char file_path[PATH_MAX];
	char buffer [MAX_BUFFER_SIZE]; //__FixMe__ size
	blob = (Blob*) malloc (MAX_BUFFER_SIZE); //__FixME__ size
		
	for (;!close_server;) {
	
		//reset variables to be used at every request
		ipc_command user_command = ipc_command_not_set;
		ZERO (file_path, PATH_MAX); //Wash file path for next request from the client
		ZERO (buffer, MAX_BUFFER_SIZE);
		recv_data (ipc_to_server);

		if (blob && strlen(blob->msg_data) > 0)
		{
			if (sscanf (blob->msg_data, "%d:%s", &user_command, file_path) != 2)
                        	user_command = ipc_command_bad;

			if (!strlen(file_path)) // If user command has initial white spaces, this may trigger
                                continue; //Trim and skip such tokens
		
			//Check if the path is valid.
			struct stat sb;
			if (-1 == stat (file_path, &sb)){
			        //file does not exits;
				sprintf (blob->msg_data, "File path \"%s\" does not exit.\nPlease check the file path and try again.", file_path);
			}
			else {
				//clean blob for sending result back
				ZERO (blob, MAX_BUFFER_SIZE);
				
				// Do the requested file op
				process_file (user_command, file_path, buffer);
				
				//Load blob
				sprintf (blob->msg_data, "%s", buffer);
                        }

			blob->msg_type = FOR_CLIENT;
                        blob->msg_len = strlen (blob->msg_data);

                        send_data (ipc_to_client);
		}
		else
		{
                        close_ipc (&ipc_to_client, ipc_open_write, IPC_PATH_TO_CLIENT); //Client writer
			close_ipc (&ipc_to_server, ipc_open_read, IPC_PATH_TO_SERVER); //Server reader
			break;
		}
	}

        return;
}


void client (ipc_mode_t ipc_mode, ipc_t ipc_to_server, ipc_t ipc_to_client, pid_t pid_server)
{
	bool close_client = false;
	char buffer [500]; //__FixMe__ size
	blob = (Blob*) malloc (MAX_BUFFER_SIZE); //__FixME__ size
	
	sleep (1); // give chance to "UP" the server
        printf ("\nThe File Server is up now. Enter commands to access the server.\n");
        PRINT_USAGE;
	
	for (;!close_client;) {
		printf ("$ ");
	
		if (fgets (buffer, MAX_BUFFER_SIZE, stdin)) // Limited size command. Assuming that file path won't excceed PATH_MAX charecters
		{
			ipc_command user_command = ipc_command_not_set;
			char* token = strtok (buffer, " \n"); //breaking the incoming commands from user with boundary as ' ' or '\n'

			//printf ("Token %s", token); //For debug puropose
			if (!token) // If user command has initial white spaces, this may trigger
				continue; //Trim and skip such tokens 				
			interpret_command (token, &user_command);
			
			switch (user_command)
			{
				case ipc_command_read:
				case ipc_command_delete:
				{	
					//It is obvious that the these commands require additional parsing of the arguments
					token = strtok (NULL, " \n");
					if (!token)
					{
						BAD_USAGE (program_name, token);
						continue;
					}
					break;        
				}
				case ipc_command_clear:
					system("clear");
					continue;
				case ipc_command_ls:
				{
					char command_result [MAX_BUFFER_SIZE];
					memset (command_result, 0, MAX_BUFFER_SIZE);

					FILE* file = popen (COMMAND_LS, "r");
					while (fread (command_result, sizeof(char), MAX_BUFFER_SIZE, file))
					{
						printf ("%s", command_result);
						memset (command_result, 0, MAX_BUFFER_SIZE);
					}
					
					pclose (file);
					continue;
				}
				case ipc_command_help:
					PRINT_USAGE;
					continue;
				case ipc_command_exit: //If user enters exit command
				{
					//Cleanup, kill Server and die

					close_ipc (&ipc_to_server, ipc_open_write, IPC_PATH_TO_SERVER); //Server writer
		                        close_ipc (&ipc_to_client, ipc_open_read, IPC_PATH_TO_CLIENT); //Client reader
					
					//Give Server some time to close the IPC before signalling
					//Actually, it is not really necessary to kill -9 server
					sleep (1);
					printf ("\nTerminating now... \n");
					//kill (pid_server, 9);
					close_client = true;
					continue;
				}
				case ipc_command_not_set:
				case ipc_command_bad:
				default:
					BAD_USAGE (program_name, token);
					continue;
			}
		
			ZERO (blob, MSG_MAX);
		        blob->msg_type = FOR_SERVER;
        		sprintf (blob->msg_data, "%d:%s", user_command, token);
        		blob->msg_len = strlen (blob->msg_data);
	
			//printf ("Token %s", token); //For debug purpose
			send_data (ipc_to_server);
			recv_data (ipc_to_client);
			display_result ();
		}

		printf ("\n");
	}

	/*if (buffer)
                free (buffer);*/

        return;
}

void interpret_command (char* token, ipc_command *user_command)
{
	*user_command = ipc_command_not_set;

	if (0 == strcmp (token, "READ"))
		*user_command = ipc_command_read;
	else if (0 == strcmp (token, "DELETE"))
		*user_command = ipc_command_delete;
	else if (0 == strcmp (token, "EXIT"))
		*user_command = ipc_command_exit;
	else if (0 == strcmp (token, "CLEAR"))
		*user_command = ipc_command_clear;
	else if (0 == strcmp (token, "LS"))
		*user_command = ipc_command_ls;
	else if (0 == strcmp (token, "HELP"))
		*user_command = ipc_command_help;
	else
		*user_command = ipc_command_bad;	
}

void send_data (ipc_t ipc_type)
{
	switch (ipc_mode)
	{
		case ipc_mode_pipe:
			if (-1 == write (ipc_type.pipe[ipc_open_write], blob, MAX_BUFFER_SIZE))
				printf ("\nError reported %s", strerror(errno));
		break;
		case ipc_mode_fifo:
			if (-1 == write (ipc_type.fifo, blob, MAX_BUFFER_SIZE))
                                printf ("\nError reported %s", strerror(errno));

		break;
		case ipc_mode_systemV_msg_q:
			if (msgsnd (ipc_type.systemv_msg_q, blob, MAX_BUFFER_SIZE, 0) < 0)
				printf ("\nError reported %s", strerror(errno));
		break;
		case ipc_mode_posix_msg_q:
		{
			if (mq_send(ipc_type.posix_msg_q, blob->msg_data, blob->msg_len, 0) < 0)
				printf ("\nError reported %s", strerror(errno));
		}
		break;
	}
}

void recv_data (ipc_t ipc_type)
{
        ZERO (blob, MAX_BUFFER_SIZE); // Wash the blob before reusing, don't want any garbage - ironical isn't it ;-)

        switch (ipc_mode)
        {
                case ipc_mode_pipe:
                        if (-1 == read (ipc_type.pipe[ipc_open_read], blob, MAX_BUFFER_SIZE))
        	                printf ("\nError reported %s", strerror(errno));	
                break;
                case ipc_mode_fifo:
			if (-1 == read (ipc_type.fifo, blob, MAX_BUFFER_SIZE))
                                printf ("\nError reported %s", strerror(errno));
                break;
                case ipc_mode_systemV_msg_q:
			if (msgrcv (ipc_type.systemv_msg_q, blob, MAX_BUFFER_SIZE, 0, 0) < 0 && errno != EIDRM)
				printf ("\nError reported %s", strerror(errno));
                break;
                case ipc_mode_posix_msg_q:			
		{
			blob->msg_len = MAX_BUFFER_SIZE;
			if (mq_receive (ipc_type.posix_msg_q, blob->msg_data, blob->msg_len, 0) < 0)
                                printf ("\nError reported %s", strerror(errno));
		}
                break;
        }
}

void display_result ()
{
	if (blob)
		printf (" Response received from Server: %s \n", blob->msg_data);	 
}

void process_file (ipc_command user_command, char* file_path, char* buffer)
{
	FILE* file_to_process = NULL;
	switch (user_command)
	{
	case ipc_command_read:
		file_to_process = fopen (file_path, "r");
		if (file_to_process == NULL)
			sprintf (buffer, "We have encountered some error while reading the file - %s.\nPlease make sure that this file is readable by current user", file_path);
		else {
			
			fread (buffer, MAX_BUFFER_SIZE, sizeof(char), file_to_process); //Read the file and write the o/p to the ipc
		}
		break;
	case ipc_command_delete:
		if (0 == remove (file_path)) //Perform the file operation and write the result in a file
			sprintf (buffer, "File deleted!!!");
		else
			sprintf (buffer, "Unable to delete the file!!!");

		break;
	case ipc_command_bad:
		sprintf (buffer, "Bad command or file name"); //LOL. I am trying to be nostalgic here
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

void close_ipc (ipc_t* ipc_type, ipc_open_mode_t mode, const char* ipc_path) 
{
	switch (ipc_mode)
        {
                case ipc_mode_pipe:
			close ((*ipc_type).pipe [mode]);
                break;
                case ipc_mode_fifo:
			close ((*ipc_type).fifo);
			unlink(ipc_path);
                break;
                case ipc_mode_systemV_msg_q:
			if ((*ipc_type).systemv_msg_q > 0)
			{
				msgctl ((*ipc_type).systemv_msg_q, IPC_RMID, NULL);
				(*ipc_type).systemv_msg_q = -1;
			}
                break;
                case ipc_mode_posix_msg_q:
		{
			//We have to put a slash in front of the ipc device name for posix msg q
			char msgq_name [50];
                        memset (msgq_name, 0, 50);
                        sprintf (msgq_name, "/%s", ipc_path);

			if ((*ipc_type).posix_msg_q > 0)
			{
				mq_close((*ipc_type).posix_msg_q);
				(*ipc_type).posix_msg_q = 0;
				mq_unlink(msgq_name);
			}
		}
                break;
        }	
}

void open_ipc (ipc_t* ipc_type, ipc_open_mode_t mode, const char* ipc_path) 
{
	switch (ipc_mode)
        {
                case ipc_mode_pipe:
			if (mode == ipc_open_read)
				close ((*ipc_type).pipe[ipc_open_write]); //Read, writing closed
			else if (mode == ipc_open_write)
                        	close ((*ipc_type).pipe[ipc_open_read]); //Write, reading closed
                break;
                case ipc_mode_fifo:
			if (mode == ipc_open_read)
				(*ipc_type).fifo = open (ipc_path, O_RDONLY, 0);
			else if (mode == ipc_open_write)
				(*ipc_type).fifo = open (ipc_path, O_WRONLY, 0);
                break;
                case ipc_mode_systemV_msg_q:
		{
			key_t key = ftok ("/tmp", 'a'); //id is any value to generate same key everytime. lsb 8 bits shouldn't be zero	
			if (-1 == key)
                        {
				printf (IPC_OPEN_ERR, strerror(errno));
				break;
                        }

                        (*ipc_type).systemv_msg_q = msgget (key, IPC_CREAT | FILE_MODE);

                        if (-1 == (*ipc_type).systemv_msg_q)
                        {
                                printf (IPC_OPEN_ERR, strerror(errno));
                        }
                }

                break;
                case ipc_mode_posix_msg_q:
		{
			char msgq_name [50];
		        memset (msgq_name, 0, 50);
        		sprintf (msgq_name, "/%s", ipc_path);

			struct mq_attr attr;  
			attr.mq_flags = 0;  
			attr.mq_maxmsg = 10;  
			attr.mq_msgsize = 500;  
			attr.mq_curmsgs = 0;
				
			(*ipc_type).posix_msg_q = mq_open (msgq_name, O_RDWR|O_CREAT, FILE_MODE, &attr);
			
			if (-1 == (*ipc_type).posix_msg_q)
			{
				printf (IPC_OPEN_ERR, strerror(errno));
			}
		}
                break;		
        }
	
}

bool create_ipc (ipc_t* ipc_type)
{
	bool ret_val = true;
	switch (ipc_mode)
        {
                case ipc_mode_pipe:
                        if (-1 == pipe ((*ipc_type).pipe))
                	{
        	                printf (IPC_OPEN_ERR, strerror(errno));
                        	ret_val = false;                        	
	                }
			
                break;
                case ipc_mode_fifo:
                        if (mkfifo (IPC_PATH_TO_SERVER, FILE_MODE) && errno != EEXIST)
                        {
                                printf (IPC_OPEN_ERR, strerror(errno));
                                ret_val = false;                               
                        }
			if (mkfifo (IPC_PATH_TO_CLIENT, FILE_MODE) && errno != EEXIST)
                        {
				unlink(IPC_PATH_TO_SERVER);
                                printf (IPC_OPEN_ERR, strerror(errno));
                                ret_val = false;
                        }
                break;
                case ipc_mode_systemV_msg_q:
                break;
                case ipc_mode_posix_msg_q:
                break;
        }

	return ret_val;
}

