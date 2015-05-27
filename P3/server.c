/***********************************************************************************
 * Unix Network Programming - Progarmming Assignment 3
 * File name 	: server.c
 * Module 	: Server.out
 * Description	: This file basically handles all the server operation requested from
                  the client. Server will have minimum o/p interaction (unless fatal).
                  The output will be passed on to the client where it will be displayed 
		  to the user. This module will implement 2 IPC mechanisms - Shared 
                  Memory. 
***********************************************************************************/
#include "common_includes.h"

int shmid;
ipc_t lock;
ipc_t wait_sem;
const char* program_name;
ipc_mode_t ipc_mode = ipc_mode_not_set;

void open_ipc (ipc_t* ipc_type); //mode could be ignored. Depends upon the ipc type
void close_ipc (ipc_t* ipc_type); //mode could be ignored. Depends upon the ipc type
int get_line(char *buffer, size_t max);

void recv_data (ipc_t ipc_to_client, char* buffer);
void process_file (ipc_command user_command, char* file_path, char* buffer);
void send_data (ipc_t ipc_to_server, char const *buffer);
void server (ipc_mode_t ipc_mode);


int main (int argc, char * const *argv)
{
	int ret_val = 0;
	char cli_arg[5];
	program_name = argv [0];
	ipc_t ipc_desc;
	
	//Creating Sync Module
	lock.semaphore = sem_open(POSIX_SHM_NAME, 0);
        if (lock.semaphore == SEM_FAILED)
                CHECK ("Fatal Error: Failed to instantiate the locking mechanism");

	wait_sem.semaphore = sem_open(POSIX_SEM_WAIT, 0);
        if (wait_sem.semaphore == SEM_FAILED)
                CHECK ("Fatal Error: Failed to instantiate the wait mechanism");

	if (argc == 2)
		ipc_mode = strtol (argv[1], NULL, BASE10);
		
	//sleep (10); //__Debug__ give a chance to the client to write something

	if (ipc_mode == ipc_mode_not_set)
	{
		printf ("\nFatal Error, Invalid Server invoke. The utility will terminate now...");
		kill (getppid(), 9); //Kill the client
		sleep (1);
		exit (-1);
	}
	
	server (ipc_mode);	
	
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

void server (ipc_mode_t ipc_mode)
{
        bool close_server = false;
	ipc_t shm_desc;	
        char file_path[PATH_MAX];
        char buffer [MAX_BUFFER_SIZE]; //__FixMe__ size

	//Open shared memory
	open_ipc (&shm_desc);

        for (;!close_server;) {
		//reset variables to be used at every request
		ipc_command user_command = ipc_command_not_set;
                ZERO (file_path, PATH_MAX); //Wash file path for next request from the client
                ZERO (buffer, MAX_BUFFER_SIZE);
                
		//Wait for the lock to be free
		recv_data (shm_desc, buffer);

                if (strlen(buffer) > 0)
                {
                        if (sscanf (buffer, "%d:%s", &user_command, file_path) != 2)
                                user_command = ipc_command_bad;
			
			if (user_command == ipc_command_exit)
			{
				close_ipc (&shm_desc); //Client writer
				break;
			}
			
                        if (!strlen(file_path)) // If user command has initial white spaces, this may trigger
                                continue; //Trim and skip such tokens

			//Check if the path is valid.
			struct stat sb;
                        if (-1 == stat (file_path, &sb)){
				 //file does not exits;
				  sprintf (buffer, "File path \"%s\" does not exit.\nPlease check the file path and try again.", file_path);
                        }
			else if (sb.st_size > MAX_BUFFER_SIZE)
				sprintf (buffer, "Size of File \"%s\" is bigger than the shared memory. Current code design doesn't support large files.", file_path);
                        else {
				//clean blob for sending result back
				ZERO (buffer, MAX_BUFFER_SIZE);
				
				// Do the requested file op
				process_file (user_command, file_path, buffer);
			}
			
			//Write the data and free the memory for client to read
                        send_data (shm_desc, buffer);
			
		//	sleep(1); //give client a chance to acquire a lock
                }
                else
                {
                        close_ipc (&shm_desc); //Client writer
                        break;
                }
        }

	if (sem_close (lock.semaphore) != 0)
                CHECK ("Fatal Error: Failed to clock the locking mechanism");

	if (sem_close (wait_sem.semaphore) != 0)
                CHECK ("Fatal Error: Failed to clock the wait mechanism");
        return;
}

void send_data (ipc_t ipc_type, char const *buffer)
{
        //At this place, it is assumem that server has  the lock
	switch (ipc_mode)
        {
                case ipc_mode_systemv_shm:
			ZERO (ipc_type.systemv_shm, MAX_BUFFER_SIZE); // Wash the buffer before reusing, don't want any garbage - ironical isn't it ;-)
                        sprintf (ipc_type.systemv_shm, "%s", buffer);
                break;
                case ipc_mode_posix_shm:
                {
			ZERO (ipc_type.posix_shm, MAX_BUFFER_SIZE); // Wash the buffer before reusing, don't want any garbage - ironical isn't it ;-)
                        sprintf (ipc_type.posix_shm, "%s", buffer);
                }
                break;
        }
	
	if (sem_post (lock.semaphore) != 0)
               CHECK ("Fatal Error: Failed to release the locking mechanism");

	//Wait till the lock is acquired by the client
	if (sem_wait (wait_sem.semaphore) != 0)
	       CHECK ("Fatal Error: Failed to acquire the waiting semaphore")
}

void recv_data (ipc_t ipc_type, char *buffer)
{
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
				unlink (POSIX_SHM_NAME); //delete the file
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
                                (*ipc_type).systemv_shm =  shmat (shmid, NULL, 0);
                        } else
                                printf (IPC_OPEN_ERR, strerror(errno));
                }

                break;
                case ipc_mode_posix_shm:
                {
                        int fd = open (POSIX_SHM_NAME, O_RDWR | O_CREAT, FILE_MODE); //= shm_open (POSIX_SHM_NAME, O_RDWR | O_CREAT, FILE_MODE);
                        if (-1 != fd)
			{
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

