/***********************************************************************************
 * Unix Network Programming - Progarmming Assignment 4
 * File name 	: server.c
 * Module 	: Server.out
 * Description	: This file basically handles all the server operation requested from
                  the client. Server will have minimum o/p interaction (unless fatal).
                  The output will be passed on to the client where it will be displayed 
		  to the user. This module will implement server sockets. Server is 
		  expected to wait for till a client successfully connects.
		  
                  I am using dynamic port allocation and host resolution for the server.
		  Therefore, when a server starts up, it displays its ip address and port
		  number. All the client must use this port number for communication.
		  Since the server is written on top of a load balancer, the IP address 
                  is expected to change too. Although the system supports both IP address 
		  and host domain name, I would recommend you to use the host domain name 
                  to resolve the server every time client tries to connect.
***********************************************************************************/
#include "common_includes.h"

/*Global Sins. Gosh, I love cpp for restricting me to use global variables*/
static struct sockaddr_in client_addr;
static int server_sock_fd = -1;
static socklen_t addr_length = 0;
static const char* program_name;
static ipc_mode_t ipc_mode = ipc_mode_not_set;

static bool open_ipc (ipc_t* ipc_type); //mode could be ignored. Depends upon the ipc type
static void close_ipc (ipc_t* ipc_type); //mode could be ignored. Depends upon the ipc type
static int get_line(char *buffer, size_t max);

static void recv_data (ipc_t ipc_to_client, char* buffer);
static void send_data (ipc_t ipc_to_server, char const *buffer);

//Server Specific
static void server (ipc_mode_t ipc_mode);
static void process_file (ipc_t sock_desc, ipc_command user_command, char* file_path, char* buffer);
static void primary_ip_addr (char const * ip_addr);

int main (int argc, char * const *argv)
{
	int ret_val = 0;
	char cli_arg[5];
	program_name = argv [0];
	ipc_t ipc_desc;
	
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
                        default:                //redundant
                                break;
                }
        }
	
	if (ipc_mode == ipc_mode_not_set)
	{
                USAGE_EXIT (argv[0]);
		fprintf (stderr, "\nTerminating now...");
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
        bool close_connection = false;
	ipc_t sock_desc; sock_desc.tcp_fd = -1;//zero is a valid descriptor	
        char file_path[PATH_MAX];
        char buffer [MAX_BUFFER_SIZE]; //__FixMe__ size

	while (1)
	{
		close_connection = false;
		fprintf (stderr, "\nWaiting for a client to connect... \nPress Ctrl+C to shutdown the server\n");

		//Open socket
		if (!open_ipc (&sock_desc))
		{			
			close_ipc (&sock_desc); //Client writer
			sleep (1);
			printf ("\nTerminating now... \n");
			break;
		}
		for (;!close_connection;) {
			//reset variables to be used at every request
			ipc_command user_command = ipc_command_not_set;
			ZERO (file_path, PATH_MAX); //Wash file path for next request from the client
			ZERO (buffer, MAX_BUFFER_SIZE);
			
			//Wait for the lock to be free
			recv_data (sock_desc, buffer);

			if (strlen(buffer) > 0)
			{
				if (sscanf (buffer, "%d:%s", &user_command, file_path) != 2)
					user_command = ipc_command_bad;

				if (!strlen(file_path)) // If user command has initial white spaces, this may trigger
					continue; //Trim and skip such tokens

				//Check if the path is valid.
				struct stat sb;
				if (-1 == stat (file_path, &sb)){
					 //file does not exits;
					  sprintf (buffer, "File path \"%s\" does not exit.\nPlease check the file path and try again.", file_path);
				}
				else {
					//clean blob for sending result back
					ZERO (buffer, MAX_BUFFER_SIZE);
					
					// Do the requested file op
					process_file (sock_desc, user_command, file_path, buffer);
				}
				
				//Write theend_data (sock_desc, buffer);data and free the memory for client to read
	//			send_data (sock_desc, buffer);
			}
			else
			{
				close_ipc (&sock_desc); //Client writer
				break;
			}
		}
	}
        return;
}

void send_data (ipc_t ipc_desc, char const *buffer)
{
	socklen_t addr_len = sizeof (client_addr);
	switch (ipc_mode)
        {
                case ipc_mode_tcp_sock:
                	send (ipc_desc.tcp_fd, buffer, MAX_BUFFER_SIZE, 0);
		break;
                case ipc_mode_udp_sock:
                {
			sendto (ipc_desc.udp_fd, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr *) &client_addr, addr_len);
                }
                break;
        }
}

void recv_data (ipc_t ipc_desc, char *buffer)
{
	socklen_t addr_len = sizeof (client_addr);
        ZERO (buffer, MAX_BUFFER_SIZE); // Wash the buffer before reusing, don't want any garbage - ironical isn't it ;-)

        switch (ipc_mode)
        {
                case ipc_mode_tcp_sock:
                	if (0 == recv (ipc_desc.tcp_fd, buffer, MAX_BUFFER_SIZE, 0))
				fprintf (stderr, "\nPeer has performed an orderedly shutdown");
			else
				fprintf (stderr, "\nServer received the following command from client: %s", buffer);
		break;
                case ipc_mode_udp_sock:
		{
			int ret_val = recvfrom (ipc_desc.udp_fd, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr *) &client_addr, &addr_len);
			if (0 == ret_val)
			{
				fprintf (stderr, "\nPeer has performed an orderedly shutdown, waiting for other client...");
			}
			else if (ret_val)
				fprintf (stderr, "\nServer received the following command from client: %s", buffer);
			else
				fprintf (stderr, "\nError occured while reading from a client");
		}
		break;
        }
}

void process_file (ipc_t sock_desc, ipc_command user_command, char* file_path, char* buffer)
{
        FILE* file_to_process = NULL;
        switch (user_command)
        {
        case ipc_command_read:
                file_to_process = fopen (file_path, "r");
                if (file_to_process == NULL)
                        sprintf (buffer, "We have encountered some error while reading the file - %s.\nPlease make sure that this file is readable by current user", file_path);
                else {
                      	while (true)
			{
				ZERO (buffer, MAX_BUFFER_SIZE);
				fread (buffer, MAX_BUFFER_SIZE, sizeof(char), file_to_process); //Read the file and write the o/p to the ipc
				//Write theend_data (sock_desc, buffer);data and free the memory for client to read
				send_data (sock_desc, buffer);

				if(feof (file_to_process))
		  			break;
			}
			
			ZERO (buffer, MAX_BUFFER_SIZE);
			sprintf (buffer, "--EOF--");
			send_data (sock_desc, buffer);
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
	int port_no, sock_type;	
        char ip_addr [INET6_ADDRSTRLEN];
	struct sockaddr_in sock_addr;
        struct addrinfo addr_info, *result_addr_info, *it_addr;
	
	if (server_sock_fd == -1) // Can do that coz union has only int (so any one should do)
	{
		ZERO (&addr_info, sizeof (addr_info));
		ZERO (ip_addr, INET6_ADDRSTRLEN);
		ZERO (&sock_addr, sizeof (struct sockaddr_in));
		
		sock_addr.sin_family            = AF_INET;
		sock_addr.sin_port              = htons(0); /*Assign a new port to me*/
		
		sock_type = ipc_mode == ipc_mode_tcp_sock ? SOCK_STREAM : SOCK_DGRAM;
		
		server_sock_fd = socket (AF_INET, sock_type, 0);
		if (-1 == server_sock_fd)
		{
			printf (IPC_OPEN_ERR, strerror(errno));
			return ret_val;
		}

		addr_length = sizeof (sock_addr);
		if (-1 == bind(server_sock_fd, (struct sockaddr*) &sock_addr, addr_length))
		{
			printf (IPC_OPEN_ERR, strerror(errno));
			return ret_val;
		}

		if (-1 == getsockname (server_sock_fd, (struct sockaddr*) &sock_addr,  &addr_length)) //Who am I???
		{
			printf (IPC_OPEN_ERR, strerror(errno));
			return ret_val;
		}

		port_no = ntohs (sock_addr.sin_port);
		inet_ntop (AF_INET, &sock_addr, ip_addr, INET_ADDRSTRLEN);
		char primary_ip[INET_ADDRSTRLEN];
		
		primary_ip_addr (primary_ip);
		CONNECTION_DETAILS(primary_ip, port_no);
	}

        switch (ipc_mode)
        {
                case ipc_mode_tcp_sock:
                {
			if (listen (server_sock_fd, BACKLOG) == -1)
				printf (IPC_OPEN_ERR, strerror(errno));

			ipc_desc->tcp_fd = accept (server_sock_fd, (struct sockaddr*) &sock_addr, &addr_length);
			if (ipc_desc->tcp_fd == -1)
			{
				printf (IPC_OPEN_ERR, strerror(errno));
				break;
			}
			else
				printf ("\nConnection Accepted!\nAccepting request from this client now...\n");
			
			ret_val = true;
		}

                break;
                case ipc_mode_udp_sock:
                {
			ipc_desc->udp_fd = server_sock_fd;
			ret_val = true;
		}
                break;
        }
        
        return ret_val;
}

void primary_ip_addr (char const * ip_addr) {
	struct ifaddrs * ifAddrStruct = NULL;
	struct ifaddrs * ifa = NULL;
	void * tmpAddrPtr = NULL;

	getifaddrs(&ifAddrStruct);

	for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
		if (!ifa->ifa_addr) {
			continue;
		}
		if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
		    // is a valid IP4 Address
		    tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
		    inet_ntop(AF_INET, tmpAddrPtr, ip_addr, INET_ADDRSTRLEN);
		}        
	}
	  
	if (ifAddrStruct!=NULL)
		freeifaddrs(ifAddrStruct);
	
}

