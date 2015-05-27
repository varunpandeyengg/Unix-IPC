/***********************************************************************************
 * Unix Network Programming - Progarmming Assignment 3
 * File name 	: client.c
 * Module 	: Client.out
 * Description	: Client module is responsible of taking user's request and transfering
                  it to the Server. Client also invokes the server. This module will
                  implement 2 IPC mechanisms - System V and POSIX Shared Memory. If 
	          everything works alright, you will see an internal commandline '$'. 
		  Press EXIT to terminate Posix SHM is file peristent where as sysytem 
		  V is kernel persistent. 
***********************************************************************************/
#include "common_includes.h"

int shmid;
ipc_t lock;
ipc_t wait_sem;
const char* program_name;
ipc_mode_t ipc_mode;

void display_result (const char *result);
void open_ipc (ipc_t* ipc_type); //mode could be ignored. Depends upon the ipc type
void close_ipc (ipc_t* ipc_type); //mode could be ignored. Depends upon the ipc type
int get_line(char *buffer, size_t max);

void recv_data (ipc_t ipc_to_client, char * buffer);
void interpret_command (char* token, ipc_command *user_command);
void process_file (ipc_command user_command, char* file_path, char* buffer);
void send_data (ipc_t ipc_to_server, char const *buffer);
void client (ipc_mode_t ipc_mode, pid_t pid_server);


int main (int argc, char * const *argv)
{
	int ret_val = 0;
	char cli_arg[5];	
	program_name = argv [0];

        char arg;
        while ((arg = getopt (argc, argv, STR_GETOPT)) != -1) {
                switch(arg)
                {
                        case 'i':
                                if (0 == strcmp (optarg, "POSIX_SHM"))
                                        ipc_mode = ipc_mode_posix_shm;
                                else if (0 == strcmp (optarg, "SYSTEMV_SHM"))
                                        ipc_mode = ipc_mode_systemv_shm;
                                break;
			default:                //redundant
				break;
                }
        }

        if (ipc_mode == ipc_mode_not_set)
        {
                USAGE_EXIT (argv[0]);
        }
	
	//Invoke Server
	printf ("\nWelcome to a AIST - Advance IPC Simulation Tool!\nCloning Server...\n");	
	
	//Make sure that the semaphore is not alraedy present. 
	//If so, delete it.
	lock.semaphore = sem_open(POSIX_SHM_NAME, O_CREAT | O_EXCL, FILE_MODE, 1);
        if (lock.semaphore == SEM_FAILED)
	{
		if (errno != EEXIST)
	                CHECK ("Fatal Error: Failed to instantiate the locking mechanism");

		sem_unlink (POSIX_SHM_NAME);
	}
	else
	{
		sem_close (lock.semaphore);
                sem_unlink (POSIX_SHM_NAME);
	}

	wait_sem.semaphore = sem_open (POSIX_SEM_WAIT, O_CREAT | O_EXCL, FILE_MODE, 1);
        if (wait_sem.semaphore == SEM_FAILED)
        {
                if (errno != EEXIST)
                        CHECK ("Fatal Error: Failed to instantiate the wait mechanism");

                sem_unlink (POSIX_SEM_WAIT);
        }
        else
        {
                sem_close (wait_sem.semaphore);
                sem_unlink (POSIX_SEM_WAIT);
        }
	
	//Creating Sync Module
        lock.semaphore = sem_open(POSIX_SHM_NAME, O_CREAT, FILE_MODE, 0);
	if (lock.semaphore == SEM_FAILED)
		CHECK ("Fatal Error: Failed to instantiate the locking mechanism");

	//Creating wait Module
	wait_sem.semaphore = sem_open(POSIX_SEM_WAIT, O_CREAT, FILE_MODE, 0);
        if (wait_sem.semaphore == SEM_FAILED)
                CHECK ("Fatal Error: Failed to instantiate the wait mechanism")

	//Wouldnt be needed if I create the semaphore = busy
        //This is not concurrent currently
        //Will have to do this before I invoke server
 	/*if (sem_wait (lock.semaphore) != 0)
		CHECK ("Fatal Error: Failed to instantiate the locking mechanism");*/
	
	pid_t pid_server = fork ();
	switch (pid_server)
	{
		case 0: // Server
			printf ("\nCloning Successful!\nSpawning Server to process with PID %d\n", getpid());
			sprintf (cli_arg, "%d\0", ipc_mode);
	               	execlp (SERVER_FILE_PATH, SERVER_FILE_PATH, cli_arg, (char*) NULL); //1 arguments passed to the server
	                ret_val = errno;

        	        printf ("\nFatal Error! Unable to start a server. Reason: %s\nTerminatinr now... ", strerror(errno)); // If exec returns, it is definitely an error
                	kill (getppid(), 9);
                return ret_val;

		case -1:
			printf ("\nFatal Error! Unable to invoke server. Reason: %s\nTerminatinr now... ", strerror(errno));
                        ret_val = errno; //redundant, for consistancy
                        return ret_val;
		default:
			client (ipc_mode, pid_server);
	}
	
	sem_unlink (POSIX_SHM_NAME);
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

void client (ipc_mode_t ipc_mode, pid_t pid_server)
{
        bool close_client = false;
        char buffer [MAX_BUFFER_SIZE], command[MAX_BUFFER_SIZE]; //__FixMe__ size
	ipc_t shm_desc;
	open_ipc (&shm_desc);
	
	sleep(1);
	
        printf ("\nThe File Server is up now. Enter commands to access the server.\n");
        PRINT_USAGE;

        for (;!close_client;) {
                printf ("$ ");
		ZERO (command, MAX_BUFFER_SIZE);
                if (fgets (command, MAX_BUFFER_SIZE, stdin)) // Limited size command. Assuming that file path won't excceed PATH_MAX charecters
                {
                        ipc_command user_command = ipc_command_not_set;
                        char* token = strtok (command, " \n"); //breaking the incoming commands from user with boundary as ' ' or '\n'
			if (!token) // If user command has initial white spaces, this may trigger
                                continue; //Trim and skip such tokens
                       
			//Interpret the command entered by the user 
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
					
					ZERO (buffer, MAX_BUFFER_SIZE);
                        		sprintf (buffer, "%d:dummyfile", user_command);
					send_data (shm_desc, buffer);

                                        printf ("\nTerminating now... \n");
					sleep (1);
					close_ipc (&shm_desc); 
					//Give Server some time to close the IPC before signalling
					//Actually, it is not really necessary to kill -9 server
					close_client = true;
                                        continue;
                                }
                                case ipc_command_not_set:
                                case ipc_command_bad:
                                default:
                                        BAD_USAGE (program_name, token);
                                        continue;
                        }

                        ZERO (buffer, MAX_BUFFER_SIZE);
                        sprintf (buffer, "%d:%s", user_command, token);
			//printf ("Token %s", token); //For debug purpose
			
			// Write the data and release the lock for server here
			send_data (shm_desc, buffer);
			
			//`sleep (1); //give server a chance to get the lock			
			// Wait for the semaphore. Once acquired, read the data from the shm				
                        recv_data (shm_desc, buffer);
                        
			//And of course show that to the user
			display_result (buffer);
                }

                printf ("\n");
        }
 	/*if (buffer)
 		free (buffer);*/
	
	// close the semaphore
	sem_close (lock.semaphore);
	sem_close (wait_sem.semaphore);
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

void send_data (ipc_t ipc_type, char const *buffer)
{
	//At this place, it is assumem that client has the lock	
        switch (ipc_mode)
        {
                case ipc_mode_systemv_shm:
			ZERO (ipc_type.systemv_shm, MAX_BUFFER_SIZE); // Wash the buffer before reusing, don't want any garbage - ironical isn't it ;-)
                        strcpy (ipc_type.systemv_shm, buffer);
                break;
                case ipc_mode_posix_shm:
                {
			ZERO (ipc_type.posix_shm, MAX_BUFFER_SIZE); // Wash the buffer before reusing, don't want any garbage - ironical isn't it ;-)
                        strcpy (ipc_type.posix_shm, buffer);
                }
                break;
        }
	
	//Release the lock
	if (sem_post (lock.semaphore) != 0)
                CHECK ("Fatal Error: Failed to release the locking mechanism");

	//Wait till the lock is acquired by the server
	if (sem_wait (wait_sem.semaphore) != 0)
	        CHECK ("Fatal Error: Failed to acquire the waiting semaphore")
}

void recv_data (ipc_t ipc_type, char *buffer)
{
	//Acuire the lock
	if (sem_wait (lock.semaphore) != 0)
                CHECK ("Fatal Error: Failed to acquire the locking mechanism");
	
	//post that you have acquired the lock
	if (sem_post (wait_sem.semaphore) != 0)
	        CHECK ("Fatal Error: Failed to release the wait mechanism");

        ZERO (buffer, MAX_BUFFER_SIZE); // Wash the buffer before reusing, don't want any garbage - ironical isn't it ;-)

        switch (ipc_mode)
        {
                case ipc_mode_systemv_shm:
			sprintf (buffer, "%s", ipc_type.systemv_shm);
                break;
                case ipc_mode_posix_shm:
                        sprintf (buffer, "%s", ipc_type.posix_shm);
                break;
        }
}

void display_result (const char const *buffer)
{
	printf (" Response received from Server: %s \n", buffer);//blob->msg_data);
}

void close_ipc (ipc_t* ipc_type)
{
	switch (ipc_mode)
        {
                case ipc_mode_systemv_shm:
                        if ((*ipc_type).systemv_shm != NULL)
                        {
                              	shmctl (shmid, IPC_RMID, NULL);
                                (*ipc_type).systemv_shm = NULL;
                        }
                break;
                case ipc_mode_posix_shm:
                {
                        if ((*ipc_type).posix_shm != NULL)
                        {
                                munmap ((*ipc_type).posix_shm, MAX_BUFFER_SIZE);
                                (*ipc_type).posix_shm = 0;
				unlink (POSIX_SHM_NAME); //DELETE THE FILE WHEN DONE
                                //shm_unlink (POSIX_SHM_NAME);
                        }
                }
                break;
        }
}

void open_ipc (ipc_t* ipc_type)
{
        switch (ipc_mode)
        {
                case ipc_mode_systemv_shm:
                {
                        key_t key = ftok (SYSV_SHM_NAME, 'x'); //id is any value to generate same key everytime. lsb 8 bits shouldn't be zero
                        if (-1 == key)
                        {
                                printf (IPC_OPEN_ERR, strerror(errno));
                                break;
                        }

                        shmid = shmget (key, MAX_BUFFER_SIZE, IPC_CREAT | FILE_MODE);
                        if (-1 != shmid)
			{
				(*ipc_type).systemv_shm = shmat (shmid, NULL, 0);
			} else
                                printf (IPC_OPEN_ERR, strerror(errno));
                }

                break;
                case ipc_mode_posix_shm:
                {
                        int fd = open (POSIX_SHM_NAME, O_RDWR|O_CREAT, FILE_MODE);//= shm_open (POSIX_SHM_NAME, O_RDWR|O_CREAT, FILE_MODE);
                        if (-1 != fd)
			{
				if (ftruncate(fd, MAX_BUFFER_SIZE) != 0)
         				CHECK("Fatal Error. Unable to increase the size the memory");

				(*ipc_type).posix_shm = mmap (NULL, MAX_BUFFER_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
				if ((*ipc_type).posix_shm == MAP_FAILED)
					CHECK("Fatal Error. Unable to map the memory");

                                close (fd); //Once mappse in m/t we dont need the descriptor
			} else
			        printf (IPC_OPEN_ERR, strerror(errno));
                }
                break;
        }
}

