/**
 * @balakri2_assignment1
 * @author  Aravind Balakrishnan <balakri2@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <ctype.h>

#include "../include/global.h"
#include "../include/logger.h"
#include "../include/structures.h"

#define TRUE 1
#define MSG_SIZE 256
#define BUFFER_SIZE 256
#define STDIN 0

int login_check = 0;

int connect_to_host(char *server_ip, int port, int server_port);

int PortValidation(char *port);
void updateBlockStat(int block, char *IPaddress, peerSideList** head);
int isBlocked(char *IPaddress, peerSideList** head);
void emptyMyList(peerSideList** head_ref);
void addToClientList(char buffer[256], peerSideList** head);
void displayIP();
int existInClientList(char IPAddress[50], peerSideList** head);
int IPValidation(char *ip_str);

void clientAuthor(char **argv);
void clientIP(char **argv);
void clientPort(char **argv, int port);

int Server(int argc, char **argv, int port);
int Client(int argc, char **argv, int port);

void CheckShellInput(int argc, char **argv);

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
int main(int argc, char **argv)
{
	/*Init. Logger*/
	cse4589_init_log(argv[2]);

	/*Clear LOGFILE*/
	fclose(fopen(LOGFILE, "w"));

	/*Start Here*/
	// The input from terminal is read and checked
	// For argument "s" the server is called
	// For argument "c" the client is called
	CheckShellInput(argc, argv);
	return 0;
}

/*The function checks the arguments in the command from the terminal
Based on the values of the arg[1], the client and server side are called
Argc, argv and the extracted port numbers are sent to the client and server*/
void CheckShellInput(int argc, char **argv)
{

	if (strcmp(argv[1], "s") == 0)
	{
		int portNo = atoi(argv[2]);
		Server(argc, argv, portNo);
	}
	else if (strcmp(argv[1], "c") == 0)
	{
		
		int portNo = atoi(argv[2]);
		Client(argc, argv, portNo);
	}
}

int Client(int argc, char **argv, int port)
{
	//Checking if the input has 3 arguments
	if(argc != 3) {
		printf("Usage:%s [ip] [port]\n", argv[0]);
		exit(-1);
	}
	
	/*Initializing variables*/
	char* ubitname = "balakri2";
	peerSideList *myClientList = NULL;
	int head_socket, selret, sock_index, fdaccept=0, caddr_len;
	fd_set master_list, watch_list;
	FD_ZERO(&master_list);
    FD_ZERO(&watch_list);
    FD_SET(STDIN, &master_list);
	head_socket = STDIN;
	int server;
	
	
	while(TRUE){

		memcpy(&watch_list, &master_list, sizeof(master_list));
		
		printf("\n[PA1-Client@CSE489/589]$ ");
		fflush(stdout);
		
		/*Reference for select() system call : https://www.softprayog.in/programming/socket-programming-using-the-select-system-call*/
		selret = select(head_socket + 1, &watch_list, NULL, NULL, NULL);
        if(selret < 0)
            perror("select failed.");
            
        if(selret > 0)
        {
			for(sock_index=0; sock_index<=head_socket; sock_index+=1){
				
					if(FD_ISSET(sock_index, &watch_list))
					{
							if (sock_index == STDIN) {
									char *msg = (char*) malloc(sizeof(char)*MSG_SIZE);
									memset(msg, '\0', MSG_SIZE);
									if(fgets(msg, MSG_SIZE-1, stdin) == NULL) //Mind the newline character that will be written to msg
										exit(-1);
									
									/*Handling new line character by replacing with null character*/
									if(msg[strlen(msg)-1] == '\n')
									{
										msg[strlen(msg)-1] = '\0';
									}
									
									char *sendMessage = (char*) malloc(sizeof(char)*MSG_SIZE);
									memset(sendMessage, '\0', MSG_SIZE);
									strncpy(sendMessage, msg, strlen(msg));

									if(strlen(msg) == 0)
									{
										continue;
									}
									
									char* token = strtok(msg, " ");
									int argc = 0;
									char* argv[1000];
									memset(argv, 0, sizeof(argv));

									/*Parsing the arguments in the commands using token
									Reference for strtok: Reference : https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm*/

									while(token) {
										argv[argc] = malloc(strlen(token) + 1);
										strcpy(argv[argc], token);
										argc += 1;
										token = strtok(NULL, " ");
									}

									/*Implementation of AUTHOR command*/
									
									if(strcmp(argv[0], "AUTHOR") == 0)
									{
										clientAuthor(argv);
									} 

									/*Implementation of IP command*/
									else if(strcmp(argv[0], "IP") == 0)
									{
										clientIP(argv);
									} 

									/*Implementation of PORT command*/
									else if (strcmp(argv[0], "PORT") == 0)
									{
										clientPort(argv, port);
									}
									
									/*Implementation of LIST command*/
									else if(strcmp(argv[0], "LIST") == 0)
									{
										cse4589_print_and_log("[%s:SUCCESS]\n", argv[0]);
										peerSideList *temp = myClientList;
										int list_id = 1;
										while(temp != NULL)
										{
											cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", list_id, temp->hostname, temp->IPaddress, temp->port);
											temp = temp->next;
											list_id += 1;
										}
										cse4589_print_and_log("[%s:END]\n", argv[0]);
									}
									
									/*Implementation of LOGIN command*/
									else if(strcmp(argv[0], "LOGIN") == 0)
									{
										cse4589_print_and_log("[%s:SUCCESS]\n", argv[0]);
										cse4589_print_and_log("[%s:END]\n", argv[0]);
										char *backup = (char*) malloc(sizeof(char)*strlen(argv[1]));
										strncpy(backup, argv[1], strlen(argv[1]));
										if(IPValidation(backup) && PortValidation(argv[2]))
										{ 
											server = connect_to_host(argv[1], port, atoi(argv[2]));
											char *buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
											memset(buffer, '\0', BUFFER_SIZE);
											myClientList == NULL;
											
											if(recv(server, buffer, BUFFER_SIZE, 0) >= 0){
												printf("Server responded: %s", buffer);
												login_check = 1;
												addToClientList(buffer, &myClientList);
												fflush(stdout);
											} 
											FD_SET(server, &master_list);
											if(server > head_socket) head_socket = server;
										}
										else {
											cse4589_print_and_log("[%s:ERROR]\n", argv[0]);
											cse4589_print_and_log("[%s:END]\n", argv[0]);
										} 
									}
									
									/*Implementation of BROADCAST command*/
									else if(strcmp(argv[0], "BROADCAST") == 0)
									{
										if(send(server, sendMessage, strlen(sendMessage), 0) == strlen(sendMessage))
											// printf("Done!\n");
											cse4589_print_and_log("[%s:SUCCESS]\n", argv[0]);
											cse4589_print_and_log("[%s:END]\n", argv[0]);
										fflush(stdout);
									} 
									
									/*Implementation of SEND command*/
									else if(strcmp(argv[0], "SEND") == 0)
									{
										char *backup = (char*) malloc(sizeof(char)*strlen(argv[1]));
										strncpy(backup, argv[1], strlen(argv[1]));
										if(IPValidation(backup) && existInClientList(argv[1], &myClientList))
										{
											if(send(server, sendMessage, strlen(sendMessage), 0) == strlen(sendMessage))
											printf("Done!\n");
											
											cse4589_print_and_log("[%s:SUCCESS]\n", argv[0]);
											cse4589_print_and_log("[%s:END]\n", argv[0]);
										}
										else 
										{
											cse4589_print_and_log("[%s:ERROR]\n", argv[0]);
											cse4589_print_and_log("[%s:END]\n", argv[0]);
										}
										fflush(stdout); 
									} 

									/*Implementation of BUFFER command*/
									else if(strcmp(argv[0], "BUFFER") == 0)
									{
										char *buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
										memset(buffer, '\0', BUFFER_SIZE);
										if(recv(server, buffer, BUFFER_SIZE, 0) >= 0){
											printf("Server responded: %s", buffer);
											login_check = 1;
											addToClientList(buffer, &myClientList);
											fflush(stdout);
										} 
									}

									/*Implementation of REFRESH command*/
									else if(strcmp(argv[0], "REFRESH") == 0)
									{
										if(send(server, sendMessage, strlen(sendMessage), 0) == strlen(sendMessage))
										{
											// printf("Done!\n");
										}
										char *buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
										memset(buffer, '\0', BUFFER_SIZE);
										
										if(recv(server, buffer, BUFFER_SIZE, 0) >= 0){
											// printf("Server responded: %s", buffer);
											emptyMyList(&myClientList);
											if(myClientList != NULL)
											{
												// printf("\nList is not empty\n");
											}
											addToClientList(buffer, &myClientList);
											fflush(stdout);
										} 
									}

									/*Implementation of BLOCK command*/
									else if(strcmp(argv[0], "BLOCK") == 0)
									{
										char *backup = (char*) malloc(sizeof(char)*strlen(argv[1]));
										strncpy(backup, argv[1], strlen(argv[1]));
										if(IPValidation(backup) && existInClientList(argv[1], &myClientList) && isBlocked(argv[1], &myClientList) == 0)
										{
											if(send(server, sendMessage, strlen(sendMessage), 0) == strlen(sendMessage))
												// printf("Done!\n");
											updateBlockStat(1, argv[1], &myClientList);
											cse4589_print_and_log("[%s:SUCCESS]\n", argv[0]);
											cse4589_print_and_log("[%s:END]\n", argv[0]);
									   }
									   else
									   {
										   cse4589_print_and_log("[%s:ERROR]\n", argv[0]);
										   cse4589_print_and_log("[%s:END]\n", argv[0]);
									   }
									}

									/*Implementation of UNBLOCK command*/
									else if(strcmp(argv[0], "UNBLOCK") == 0)
									{
										char *backup = (char*) malloc(sizeof(char)*strlen(argv[1]));
										strncpy(backup, argv[1], strlen(argv[1]));
										if(IPValidation(backup) && existInClientList(argv[1], &myClientList) && isBlocked(argv[1], &myClientList) == 1)
										{
											if(send(server, sendMessage, strlen(sendMessage), 0) == strlen(sendMessage))
												// printf("Done!\n");
											updateBlockStat(0, argv[1], &myClientList);
											cse4589_print_and_log("[%s:SUCCESS]\n", argv[0]);
											cse4589_print_and_log("[%s:END]\n", argv[0]);
										}
										else
										{
										   cse4589_print_and_log("[%s:ERROR]\n", argv[0]);
										   cse4589_print_and_log("[%s:END]\n", argv[0]);
										}
										
									}
									
							}
							else
							{
								/* Initialize buffer to receieve response */
								char *buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
								memset(buffer, '\0', BUFFER_SIZE);

								if(recv(sock_index, buffer, BUFFER_SIZE, 0) <= 0){
									close(sock_index);
									printf("Remote Host terminated connection!\n");

									/* Remove from watched list */
									FD_CLR(sock_index, &master_list);
								}
								else {
									//Process incoming data from existing server here ...
									char delimiter[] = " ";
									char *start, *second, *remainder, *context;
									printf("\nServer sent me: %s\n", buffer);
									start = strtok_r (buffer, delimiter, &context);
									second = strtok_r (NULL, delimiter, &context);
									remainder = context;
									printf("first %s\n", start);
									printf("second %s\n", second);
									printf("remainder %s\n", remainder);
									cse4589_print_and_log("[RECEIVED:SUCCESS]\n");
									cse4589_print_and_log("msg from:%s\n[msg]:%s\n", second, remainder);
									cse4589_print_and_log("[RECEIVED:END]\n"); 
									
								}

								free(buffer);
							}
				}
			}
		}
	}
}

int connect_to_host(char *server_ip, int port, int server_port)
{
	int fdsocket, len;
	struct sockaddr_in remote_server_addr;

	fdsocket = socket(AF_INET, SOCK_STREAM, 0);
	if (fdsocket < 0)
		perror("Failed to create socket");

	
	/*Reference used: https://man7.org/linux/man-pages/man3/bzero.3.html*/
	bzero(&remote_server_addr, sizeof(remote_server_addr));
	remote_server_addr.sin_family = AF_INET;
	inet_pton(AF_INET, server_ip, &remote_server_addr.sin_addr);
	remote_server_addr.sin_port = htons(server_port);

	if (connect(fdsocket, (struct sockaddr *)&remote_server_addr, sizeof(remote_server_addr)) < 0)
		perror("Connect failed");
	char portNo[100];
	memset(portNo, 0, sizeof(portNo));

	/*Reference used: */
	sprintf(portNo, "%d", port);
	if (send(fdsocket, portNo, strlen(portNo), 0) == strlen(portNo))
		printf("Done!\n");
	fflush(stdout);
	cse4589_print_and_log((char *)"[%s:SUCCESS]\n", "LOGIN");
	cse4589_print_and_log((char *)"[%s:END]\n", "LOGIN");

	return fdsocket;
}

int valid_digit(char *ip_str)
{
	/*Reference used: https://tutorialspoint.dev/language/c/program-to-validate-an-ip-address */
	while (*ip_str)
	{
		if (*ip_str >= '0' && *ip_str <= '9')
			++ip_str;
		else
			return 0;
	}
	return 1;
}

int IPValidation(char *ip_str)
{
	/*Reference used: https://tutorialspoint.dev/language/c/program-to-validate-an-ip-address */
	int i, num, dots = 0;
	char *ptr;
	printf("\ninside is valid\n");
	if (ip_str == NULL)
		return 0;

	// Reference : https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm
	ptr = strtok(ip_str, ".");

	if (ptr == NULL)
		return 0;

	while (ptr)
	{

		/* Ensuring onlydigits are present */
		if (!valid_digit(ptr))
			return 0;

		num = atoi(ptr);

		/* IP validation */
		if (num >= 0 && num <= 255)
		{
			/* parsing through the string */
			ptr = strtok(NULL, ".");
			if (ptr != NULL)
				++dots;
		}
		else
			return 0;
	}

	/* valid IP string must contain 3 dots */
	if (dots != 3)
		return 0;
	return 1;
}

int PortValidation(char *port)
{
	while (*port)
	{
		if (isdigit(*port++) == 0)
			return 0;
	}
	return 1;
}

void clientAuthor(char **argv)
{
    char* ubitname = "balakri2";
    cse4589_print_and_log("[%s:SUCCESS]\n", argv[0]);
    cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", ubitname);
    cse4589_print_and_log("[%s:END]\n", argv[0]);
}

void clientIP(char **argv)
{
    cse4589_print_and_log("[%s:SUCCESS]\n", argv[0]);
    displayIP();
    cse4589_print_and_log("[%s:END]\n", argv[0]);
}

void clientPort(char **argv, int port)
{
    cse4589_print_and_log("[%s:SUCCESS]\n", argv[0]);
    cse4589_print_and_log("PORT:%d\n", port);
    cse4589_print_and_log("[%s:END]\n", argv[0]);
}

// 	References :
// 	1. https://github.com/Kamalesh1497/CSE589-Modern-Networking-Concepts
// 	2. https://github.com/Vaishnavi3799/Text_Chat_Application
// 	3. https://github.com/shaduk/Text-Chat-Application-over-TCP
//  4. https://github.com/vaibhavchincholkar/CSE-489-589-Modern-Networking-Concepts
