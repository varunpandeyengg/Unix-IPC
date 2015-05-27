#include <sys/stat.h>
#include <stdlib.h>
#include "file_op.h"

file_result str_result;
#define MAX_BUFFER_SIZE 50000
#define ZERO(buff, size) memset (buff, 0, size);
typedef enum
{
        ipc_command_read,
        ipc_command_delete,
        ipc_command_bad
} ipc_command;

void process_file (ipc_command user_command, char* file_path, char* buffer);

file_result * read_file_1_svc(char ** ppath, struct svc_req *req)
{
	str_result.data = malloc(MAX_BUFFER_SIZE);
	process_file (ipc_command_read, *ppath, str_result.data);
	return &str_result;
}

file_result * delete_file_1_svc(char ** ppath, struct svc_req *req)
{
	str_result.data = malloc(MAX_BUFFER_SIZE);        
	process_file (ipc_command_delete, *ppath, str_result.data);	
        return &str_result;;
}


void process_file (ipc_command user_command, char* file_path, char* buffer)
{
	//Check if the path is valid.
	struct stat sb;
	if (-1 == stat (file_path, &sb)){
		  //file does not exits;
		  sprintf (buffer, "File path \"%s\" does not exit.\nPlease check the file path and try again.", file_path);
		  return;
	}
		
	ZERO (buffer, MAX_BUFFER_SIZE);
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
