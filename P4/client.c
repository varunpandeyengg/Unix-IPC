/***********************************************************************************
 * Unix Network Programming - Progarmming Assignment 4
 * File name 	: client.c
 * Module 	: Client.out
 * Description	: Client module is responsible of taking user's request and transfering
                  it to the Server. This module will implement Socket. You may connect to the 
		  server using IP address or server name. If everything works alright, you 
		  will see an internal commandline '$'.or server domain name. Server is expected 
		  to print its port number and IP address. User this socket address to connect.
		  Press EXIT to terminate.
***********************************************************************************/
#include "common_includes.h"

/*Global Sins. Gosh, I love cpp for restricting me to use global variables*/

static ipc_t lock;
static const char * port_no;
static const char * program_name;
static struct sockaddr_in* server_addr;

static char * server_name = "linux.dc.engr.scu.edu";
static ipc_mode_t ipc_mode = ipc_mode_not_set;

static bool open_ipc (ipc_t* ipc_type); //mode could be ignored. Depends upon the ipc type
static void close_ipc (ipc_t* ipc_type); //mode could be ignored. Depends upon the ipc type
static int get_line(char *buffer, size_t max);

static void recv_data (ipc_t ipc_to_client, char * buffer);
static void send_data (ipc_t ipc_to_server, char const *buffer);

//Client Specific
static void client (ipc_mode_t ipc_mode);
static void interpret_command (char* token, ipc_command *user_command);
static void display_result (const char *result);

int main (int argc, char * const *argv)
{
	int ret_val = 0;
	program_name = argv [0];

        char arg;
        while ((arg = getopt (argc, argv, STR_GETOPT)) != -1) {
                switch(arg)
                {
                        case 'i':
                                if (0 == strcmp (optarg, "UDP"))
                                        ipc_mode = ipc_mode_udp_sock;
                                else if (0 == strcmp (optarg, "TCP"))
                                        ipc_mode = ipc_mode_tcp_sock;
                                break;
			case 'h':
				server_name = optarg;
				break;
			case 'p':
				port_no = optarg;
			default:                //redundant
				break;
                }
        }

        if (ipc_mode == ipc_mode_not_set | !server_name | !port_no)
        {
                USAGE_EXIT (argv[0]);
        }
	
	client (ipc_mode);

	return 0;
}

static int get_line(char *buffer, size_t max)
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

static void client (ipc_mode_t ipc_mode)
{
        bool close_client = false;
        char buffer [MAX_BUFFER_SIZE], command[MAX_BUFFER_SIZE]; //__FixMe__ size
	ipc_t sock_desc;
	
	//Invoke Server
	printf ("\nWelcome to a SIST - Socket IPC Simulation Tool!\n");
	if (!open_ipc (&sock_desc)) //Open socket
	{
		//Cleanup, kill Server and die
		close_ipc (&sock_desc); 
		//Give Server some time to close the IPC before signalling
		//Actually, it is not really necessary to kill -9 server
		sleep (1);
		printf ("\nTerminating now... \n");
		close_client = true;
	}
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
					close_ipc (&sock_desc); 
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

			//format data to send
                        ZERO (buffer, MAX_BUFFER_SIZE);
                        sprintf (buffer, "%d:%s", user_command, token);
				
			// Write the data to the outgoing sockect message q
			send_data (sock_desc, buffer);
			
			// Read the data from the incoming sockect message q
                        recv_data (sock_desc, buffer);
                        
			//And of course show that to the user
			//display_result (buffer);
			//Done in recv to handle multi large data
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

void send_data (ipc_t ipc_desc, char const *buffer)
{
	socklen_t addr_len = sizeof (*server_addr);
	//At this place, it is assumem that client has the lock	
        switch (ipc_mode)
        {
                case ipc_mode_tcp_sock:
                	send (ipc_desc.tcp_fd, buffer, MAX_BUFFER_SIZE, 0);
		break;
                case ipc_mode_udp_sock:
			sendto (ipc_desc.udp_fd, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr *) server_addr, addr_len);
                break;
        }
}

void recv_data (ipc_t ipc_desc, char *buffer)
{
	socklen_t addr_len = sizeof (*server_addr);
        ZERO (buffer, MAX_BUFFER_SIZE); // Wash the buffer before reusing, don't want any garbage - ironical isn't it ;-)

        switch (ipc_mode)
        {
                case ipc_mode_tcp_sock:
                	while (true)
			{
				recv (ipc_desc.tcp_fd, buffer, MAX_BUFFER_SIZE, 0);
				if(NULL != strstr(buffer, "--EOF--"))
					break;
				
				display_result(buffer);
			}
		break;
                case ipc_mode_udp_sock:
			while (true)
                        {
				recvfrom (ipc_desc.udp_fd, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr *) server_addr, &addr_len);
				if(NULL != strstr(buffer, "--EOF--"))
                                        break;

                                display_result(buffer);
			}
                break;
        }
}

void display_result (const char const *buffer)
{
	printf ("%s", buffer);
}

void close_ipc (ipc_t* ipc_desc)
{
	switch (ipc_mode)
        {
                case ipc_mode_tcp_sock:
			close (ipc_desc->tcp_fd);
			ipc_desc->tcp_fd = -1;
		break;
                case ipc_mode_udp_sock:
			close (ipc_desc->udp_fd);
			ipc_desc->udp_fd = -1;
		break;
        }
}

bool open_ipc (ipc_t* ipc_desc)
{
	bool ret_val = false;
	int sock_fd, port_num;
	struct addrinfo addr_info, *result_addr_info, *it_addr;

	ZERO (&addr_info, sizeof (addr_info));

	addr_info.ai_family             = AF_INET;
	addr_info.ai_socktype           = ipc_mode == ipc_mode_tcp_sock ? SOCK_STREAM : SOCK_DGRAM;

	if (0 != getaddrinfo (server_name, port_no, &addr_info, &result_addr_info))
	{
		CHECK ("Fatal Error. Connection Failed!");
		close (ipc_desc->tcp_fd);
		ipc_desc->tcp_fd = -1;
		return ret_val;
	}
	//Memory Leak
	//freeaddrinfo() needed

	for (it_addr = result_addr_info; it_addr != NULL; it_addr = it_addr->ai_next)
	{
		sock_fd = socket (it_addr->ai_family, it_addr->ai_socktype, 0);
		if (ipc_desc->tcp_fd == -1)
			continue; //Try the next socket
		else
			break;
	}

	switch (ipc_mode)
        {
                case ipc_mode_tcp_sock:
                {
			ipc_desc->tcp_fd = sock_fd;
			if (connect (ipc_desc->tcp_fd, it_addr->ai_addr, it_addr->ai_addrlen) == -1) {
				CHECK ("Fatal Error. Connection Failed!");
				close (ipc_desc->tcp_fd);
				ipc_desc->tcp_fd = -1;					
			}
			
			ret_val = true;
                }

                break;
                case ipc_mode_udp_sock:
                {
			ipc_desc->udp_fd = sock_fd;
			server_addr = (struct sockaddr_in *) it_addr->ai_addr;
			
			ret_val = true;
		}
                break;
        }
        
        return ret_val;
}

