#include "common_includes.h"
#include "file_op.h"

static const char * program_name;
static char * server_name = "linux.dc.engr.scu.edu";

void client (CLIENT *cl);
void interpret_command (char* token, ipc_command *user_command);

int main(int argc, char *argv[])
{
	CLIENT *cl;	

	program_name = argv [0];
	
	char arg;
        while ((arg = getopt (argc, argv, STR_GETOPT)) != -1) {
                switch(arg)
                {                        
			case 'h':
				server_name = optarg;
				break;		
			default:                //redundant
				break;
                }
        }
                        
	cl = clnt_create(server_name, PRINTER, PRINTER_V1, "tcp");
	
	if (cl == NULL) {
		printf("error: could not connect to server.\n");
		return 1;
	}

	client(cl);
	return 0;
}

void client (CLIENT *cl)
{
        bool close_client = false;
        char command [MAX_BUFFER_SIZE]; //__FixMe__ size
		
	//Invoke Server
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
					//It is obvious that the these commands require additional parsing of the arguments
					token = strtok (NULL, " \n");
                                        if (!token)
                                        {
                                                BAD_USAGE (program_name, token);
                                                continue;
                                        }
					file_result * result = read_file_1 (&token, cl);
					printf ("%s", result->data);
					break;
                                case ipc_command_delete:
                                {
					//It is obvious that the these commands require additional parsing of the arguments
					token = strtok (NULL, " \n");
                                        if (!token)
                                        {
                                                BAD_USAGE (program_name, token);
                                                continue;
                                        }
                                        file_result * result = delete_file_1 (&token, cl);
					printf ("%s", result->data);
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
					//Give Server some time to close the IPC before signalling
					//Actually, it is not really necessary to kill -9 server
					sleep (1);
                                        printf ("\nTerminating now... \n");
					close_client = true;
                                        continue;
                                }
				
                                case ipc_command_not_set:
                                case ipc_command_bad:
                                default:
                                        BAD_USAGE (program_name, token);
                                        continue;
                        }			
                }

                printf ("\n");
        }
	
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