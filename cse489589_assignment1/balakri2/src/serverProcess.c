/**
* @server
* @author  Swetank Kumar Saha <swetankk@buffalo.edu>, Shivang Aggarwal <shivanga@buffalo.edu>
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
* This file contains the server init and main while loop for tha application.
* Uses the select() API to multiplex between network I/O and STDIN.
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>

#define BACKLOG 5
#define STDIN 0
#define TRUE 1
#define CMD_SIZE 100
#define BUFFER_SIZE 512
#define MSG_SIZE 512

#include "../include/global.h"
#include "../include/logger.h"
#include "../include/structures.h"

int IPValidation(char *ip_str);
int existInServerList(char *IPaddress, clientList** head);
void displayBlockedList(char ip[100], clientList** list);
void IPSend(char ipaddress[256], clientList** list, char sendMessage[256]);
void addToServerList(char host[200], int clientlistenport, int s, clientList** list);
void displayClientList(clientList** list);
const char *inet_ntop(int af, const void *src,
                      char *dst, socklen_t size);


void sendServerList(int socket, clientList** list);
void displayIP();

void serverAuthor(char **argv);
void serverIP(char **argv);
void serverPort(char **argv, int port);


/**
* main function
*
* @param  argc Number of arguments
* @param  argv The argument list
* @return 0 EXIT_SUCCESS
*/
int Server(int argc, char **argv, int port)
{
	int server_socket, head_socket, selret, sock_index, fdaccept=0, caddr_len;
	struct sockaddr_in server_addr, client_addr;
	fd_set master_list, watch_list;
	char *ubitname = "balakri2";
	clientList *myServerList = NULL;

	/* Socket */

	// Reference : https://www.geeksforgeeks.org/socket-programming-cc/
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket < 0)
		perror("Cannot create socket");

	/* Fill up sockaddr_in struct */
	bzero(&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    /* Bind */
    if(bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0 )
    	perror("Bind failed");

    /* Listen */
    if(listen(server_socket, BACKLOG) < 0)
    	perror("Unable to listen on port");

    /* ---------------------------------------------------------------------------- */

    /* Zero select FD sets */
    FD_ZERO(&master_list);
    FD_ZERO(&watch_list);
    
    /* Register the listening socket */
    FD_SET(server_socket, &master_list);
    /* Register STDIN */
    FD_SET(STDIN, &master_list);

    head_socket = server_socket;
	
	while(TRUE){
		memcpy(&watch_list, &master_list, sizeof(master_list));

        printf("\n[PA1-Server@CSE489/589]$ ");
		fflush(stdout);

        /* Implement select() system call*/
        selret = select(head_socket + 1, &watch_list, NULL, NULL, NULL);
        if(selret < 0)
            perror("select failed.");

        
        if(selret > 0){
            /* Check which socket descriptors are ready */
            for(sock_index=0; sock_index<=head_socket; sock_index+=1){

                if(FD_ISSET(sock_index, &watch_list)){

                    /* If a new command is entered on STDIN store in command variable */
                    if (sock_index == STDIN){
                    	char *command = (char*) malloc(sizeof(char)*CMD_SIZE);

                    	memset(command, '\0', CMD_SIZE);
						if(fgets(command, CMD_SIZE-1, stdin) == NULL) //Mind the newline character that will be written to command
							exit(-1);
							
						/*Handling new line character by replacing with null character*/
						if(command[strlen(command)-1] == '\n')
						{
							command[strlen(command)-1] = '\0';
						}
						
						/*Parsing the arguments in the commands using token
						Reference for strtok: Reference : https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm */
                   
						//Process PA1 commands here ...
                        char* token = strtok(command, " ");
						int argc = 0;
						char* argv[1000];
						
						while(token) {
							argv[argc] = malloc(strlen(token) + 1);
							strcpy(argv[argc], token);
							argc += 1;
							token = strtok(NULL, " ");
						}
						
						/*Implementation of AUTHOR command*/
						if(strcmp(argv[0], "AUTHOR") == 0)
						{
							serverAuthor(argv);
						} 

						/*Implementation of IP command*/
						else if(strcmp(argv[0], "IP") == 0)
						{
							serverIP(argv);
						} 

						/*Implementation of PORT command*/
						else if (strcmp(argv[0], "PORT") == 0)
						{
							serverPort(argv, port);
						}

						/*Implementation of LIST command*/
						else if(strcmp(argv[0], "LIST") == 0)
						{
							cse4589_print_and_log("[%s:SUCCESS]\n", argv[0]);
							displayClientList(&myServerList);
							cse4589_print_and_log("[%s:END]\n", argv[0]);
						} 

						/*Implementation of BLOCKED command*/
						else if(strcmp(argv[0], "BLOCKED") == 0)
						{
								/*backup variable is used to store the 2nd argument in the command as a copy
                                validation is done for this backup variable to ensure if the IP address extracted is valid and present in the serverlist*/
                                char *backup = (char*) malloc(sizeof(char)*strlen(argv[1]));
								strncpy(backup, argv[1], strlen(argv[1]));
								if(IPValidation(backup) && existInServerList(argv[1], &myServerList))
								{
									cse4589_print_and_log("[%s:SUCCESS]\n", argv[0]);
									displayBlockedList(argv[1], &myServerList);
									cse4589_print_and_log("[%s:END]\n", argv[0]);
								}
								else
								{
									cse4589_print_and_log("[%s:ERROR]\n", argv[0]);
									cse4589_print_and_log("[%s:END]\n", argv[0]);
								}
						}
						
						free(command);
                    }
                    /* Check if new client is requesting connection */
                    else if(sock_index == server_socket){

                        /*caddr_len stores the length of the client address requesting connection*/
                        caddr_len = sizeof(client_addr);
                        fdaccept = accept(server_socket, (struct sockaddr *)&client_addr, &caddr_len);
                        if(fdaccept < 0)
                            perror("Accept failed.");

						printf("\nRemote Host connected!\n");    
						char host[1024];
						char service[20];

						
						char *buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
                        memset(buffer, '\0', BUFFER_SIZE);
                        
						if(recv(fdaccept, buffer, BUFFER_SIZE, 0) >= 0)
						{
							printf("Client sent me listen port %s \n", buffer);
						}
						int clientListenPort =  atoi(buffer);
						addToServerList(host, clientListenPort, fdaccept, &myServerList);
						sendServerList(fdaccept, &myServerList);
						
                        /* Add to watched socket list */
                        FD_SET(fdaccept, &master_list);
                        if(fdaccept > head_socket) head_socket = fdaccept;
                    }


                    /* Read from existing clients */
                    else{
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
                        	
                        	int isBlocked = 0;
                        	char fromclientip[50];
                        	memset(fromclientip, 0, sizeof fromclientip);
                        	clientList *temp = myServerList;
                        	
							while(temp != NULL)
							{
								
								if(temp->id == sock_index)
								{
									strncpy(fromclientip, temp->IPaddress, sizeof(temp->IPaddress));
									isBlocked = temp->isBlocked;
								}
								temp = temp->next;
							}
							free(temp);
                        	char *sendMessage = (char*) calloc(strlen(buffer) + 1, sizeof(char));;
							strncpy(sendMessage, buffer, strlen(buffer));
                      		char* token = strtok(buffer, " ");
							
							char* argv[300];
							int argc = 0;
							
							while(token) {
								argv[argc] = malloc(strlen(token) + 1);
								strcpy(argv[argc], token);
								argc += 1;
								token = strtok(NULL, " ");
							}

							/*Implementation of BROADCAST command*/
							if(strcmp(argv[0], "BROADCAST") == 0)
							{
								char forwardMessage[256];
								memset(forwardMessage, 0, sizeof forwardMessage);
								//printf("ECHOing it back to the remote host and other connected devices... ");
							
								char delimiter[] = " ";
								char *start, *remainder, *context;
								start = strtok_r (sendMessage, delimiter, &context);
								remainder = context;
								printf("\nMessage for broadcast1 : %s", remainder);
								strcat(forwardMessage, "RCV ");
								strcat(forwardMessage, fromclientip);
								strcat(forwardMessage, " ");
								strcat(forwardMessage, remainder); 
								for(int i = 0; i <= head_socket; i++) 
								{
									if(FD_ISSET(i, &master_list))
									{
										if(i != 0 && i != server_socket && i != sock_index)
										{
											printf("%d", i);
											if(send(i, forwardMessage, strlen(forwardMessage), 0) == strlen(forwardMessage))
											{
												
												cse4589_print_and_log("[RELAYED:SUCCESS]\n");
												cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", fromclientip, "255.255.255.255", remainder);
												cse4589_print_and_log("[RELAYED:END]\n");
											}
												fflush(stdout);
										}	
																	
									}
								} 
							}

							/*Implementation of SEND command*/
							else if(strcmp(argv[0], "SEND") == 0)
							{
								char forwardMessage[256];
								memset(forwardMessage, 0, sizeof forwardMessage);

								//Refrence from : https://www.quora.com/How-can-I-extract-words-from-a-sentence-in-C
								if(isBlocked == 0)
								{
									char delimiter[] = " ";
									printf("whole message: %s\n", sendMessage);
									char *start, *second, *remainder, *context;
									start = strtok_r (sendMessage, delimiter, &context);
									second = strtok_r (NULL, delimiter, &context);
									remainder = context;
									strcat(forwardMessage, "RCV ");
									strcat(forwardMessage, fromclientip);
									strcat(forwardMessage, " ");
									strcat(forwardMessage, remainder);
									cse4589_print_and_log("[RELAYED:SUCCESS]\n");
									cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", fromclientip, second, remainder);
									cse4589_print_and_log("[RELAYED:END]\n");
									IPSend(second, &myServerList, forwardMessage);
								}
								
							}

							/*Implementation of REFRESH command*/
							else if(strcmp(argv[0], "REFRESH") == 0)
							{
								clientList *temp = myServerList;
								while(temp != NULL)
								{
									if(temp->id == sock_index)
									{
										sendServerList(sock_index, &myServerList);
										break;
									}
									temp = temp->next;
								}
							}

							/*Implementation of BLOCK command*/
							else if(strcmp(argv[0], "BLOCK") == 0)
							{

								char delimiter[] = " ";
								char *start, *remainder, *context;
								start = strtok_r (sendMessage, delimiter, &context);
								remainder = context;
								clientList *temp1 = myServerList;
								printf("Blocked by %s\n", fromclientip);
								while(temp1 != NULL)
								{
									if(strcmp(temp1->IPaddress, remainder) == 0)
									{
										strcpy(temp1->blockedby, fromclientip);
										temp1->isBlocked = 1;
									}
									temp1 = temp1->next;
								}
							}

							/*Implementation of UNBLOCK command*/
							else if(strcmp(argv[0], "UNBLOCK") == 0)
							{
								char delimiter[] = " ";
								char *start, *remainder, *context;
								start = strtok_r (sendMessage, delimiter, &context);
								remainder = context;
								clientList *temp = myServerList;
								while(temp != NULL)
								{
									if(strcmp(temp->IPaddress, remainder) == 0)
									{
										temp->isBlocked = 0;
									}
									temp = temp->next;
								}
							}
                        }

                        free(buffer);
                    }
                }
            }
        }
    }

    return 0;
}

void displayIP()
{
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET6_ADDRSTRLEN];
    char hostname[128];
    gethostname(hostname, sizeof hostname);
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(hostname, NULL, &hints, &res)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    }

    for (p = res; p != NULL; p = p->ai_next)
    {
        void *addr;
        char *ipver;

        if (p->ai_family == AF_INET)
        { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        }
        else
        { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }

        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        cse4589_print_and_log("IP:%s\n", ipstr);
    }

    freeaddrinfo(res); // free the linked list
}

void serverAuthor(char **argv)
{
    char* ubitname = "balakri2";
    cse4589_print_and_log("[%s:SUCCESS]\n", argv[0]);
    cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", ubitname);
    cse4589_print_and_log("[%s:END]\n", argv[0]);
}

void serverIP(char **argv)
{
    cse4589_print_and_log("[%s:SUCCESS]\n", argv[0]);
    displayIP();
    cse4589_print_and_log("[%s:END]\n", argv[0]);
}

void serverPort(char **argv, int port)
{
    cse4589_print_and_log("[%s:SUCCESS]\n", argv[0]);
    cse4589_print_and_log("PORT:%d\n", port);
    cse4589_print_and_log("[%s:END]\n", argv[0]);
}


//Function.c
int highestPort = -1;

// int getnameinfo(const struct sockaddr *sa, socklen_t salen,
//                 char *host, size_t hostlen,
//                 char *serv, size_t servlen, int flags);


void addToServerList(char host[200], int clientlistenport, int s, clientList **head)
{
    socklen_t len;
    struct sockaddr_storage addr;
    char ipstr[INET6_ADDRSTRLEN];
    int port;

    len = sizeof addr;
    getpeername(s, (struct sockaddr *)&addr, &len);

    // deal with both IPv4 and IPv6:
    if (addr.ss_family == AF_INET)
    {
        struct sockaddr_in *s = (struct sockaddr_in *)&addr;
        port = ntohs(s->sin_port);
        inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
    }
    else
    { // AF_INET6
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
        port = ntohs(s->sin6_port);
        inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
    }

    clientList *new = (clientList *)malloc(sizeof(clientList));
    clientList *current = *head;
    new->id = s;
    strncpy(new->IPaddress, ipstr, strlen(ipstr));
    strncpy(new->hostname, host, strlen(host));
    new->port = clientlistenport;
    new->isBlocked = 0;
    new->logStatus = 1;
    new->next = NULL;

    if (*head == NULL || (*head)->port > new->port)
    {
        new->next = *head;
        *head = new;
    }
    else
    {
        while (current->next != NULL && current->next->port < new->port)
        {
            current = current->next;
        }
        new->next = current->next;
        current->next = new;
    }
    printf("Peer IP address: %s\n", ipstr);
    printf("Peer Listening port      : %d\n", clientlistenport);
    printf("Host Name      : %s\n", host);
}

void IPSend(char ipaddress[256], clientList **list, char sendMessage[512])
{
    clientList *temp = *list;
    while (temp != NULL)
    {
        if (strcmp(temp->IPaddress, ipaddress) == 0)
        {
            if (send(temp->id, sendMessage, strlen(sendMessage), 0) == strlen(sendMessage))
                printf("Done!\n");
            fflush(stdout);
        }
        temp = temp->next;
    }
}

void displayClientList(clientList **list)
{
    clientList *temp = *list;
    int list_id = 1;
    while (temp != NULL)
    {
        cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", list_id, temp->hostname, temp->IPaddress, temp->port);
        list_id += 1;
        temp = temp->next;
    }
}

void displayBlockedList(char ip[100], clientList **list)
{
    clientList *temp = *list;
    int list_id = 1;
    while (temp != NULL)
    {
        if (temp->isBlocked == 1 && strcmp(temp->blockedby, ip) == 0)
        {
            cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", list_id, temp->hostname, temp->IPaddress, temp->port);
            list_id += 1;
        }
        temp = temp->next;
    }
}

void sendServerList(int socket, clientList **list)
{
    clientList *temp = *list;
    char buff[100];
    char listc[1000];
    memset(listc, 0, sizeof(listc));
    memset(buff, 0, sizeof(buff));
    while (temp != NULL)
    {
        printf("%d \n", temp->port);

        sprintf(buff, "%d-%s-%s+", temp->port, temp->IPaddress, temp->hostname);
        strcat(listc, buff);
        fflush(stdout);
        temp = temp->next;
    }
    if (send(socket, listc, strlen(listc), 0) == strlen(listc))
        printf("Done!\n");
}

void addToClientList(char buffer[256], peerSideList **head)
{
    // code taken from stack overflow https://stackoverflow.com/questions/4693884/nested-strtok-function-problem-in-c

    char *line;
    char *token;
    char buf[256];
    for (line = strtok(buffer, "+"); line != NULL;
         line = strtok(line + strlen(line) + 1, "+"))
    {
        strncpy(buf, line, sizeof(buf));
        int i = 1;
        peerSideList *last = *head;
        peerSideList *new = (peerSideList *)malloc(sizeof(peerSideList));

        for (token = strtok(buf, "-"); token != NULL;
             token = strtok(token + strlen(token) + 1, "-"))
        {
            if (i == 1)
            {
                new->port = atoi(token);
                i += 1;
            }
            else if (i == 2)
            {
                memset(new->IPaddress, 0, 100);
                strncpy(new->IPaddress, token, strlen(token));
                i += 1;
            }
            else if (i == 3)
            {
                memset(new->hostname, 0, 256);
                strncpy(new->hostname, token, strlen(token));
                i += 1;
            }
        }
        new->isBlocked = 0;
        // printf("\nPort is %d\n", new->port);
        // printf("\nIP address %s\n", new->IPaddress);
        // printf("\nHost Is %s\n", new->hostname);
        new->next = NULL;
        if (*head == NULL)
        {
            *head = new;
        }
        else
        {
            while (last->next != NULL)
            {
                last = last->next;
            }
            last->next = new;
        }
    }
}

//code referred from geeks for geeks  http://www.geeksforgeeks.org/write-a-function-to-delete-a-linked-list/

void emptyMyList(peerSideList **head_ref)
{
    /* deref head_ref to get the real head */
    peerSideList *current = *head_ref;
    peerSideList *next;

    while (current != NULL)
    {
        next = current->next;
        free(current);
        current = next;
    }

    /* deref head_ref to affect the real head back
      in the caller. */
    *head_ref = NULL;
}

int existInClientList(char *IPaddress, peerSideList **head)
{
    printf("\nInside is client list function\n");
    peerSideList *temp = *head;
    while (temp != NULL)
    {
        printf("string 1: %s string 2: %s\n", temp->IPaddress, IPaddress);
        printf("comparison %d\n", strcmp(temp->IPaddress, IPaddress));
        if (strcmp(temp->IPaddress, IPaddress) == 0)
        {
            printf("I am returned 1\n");
            return 1;
        }
        temp = temp->next;
    }
    printf("I am returned 0\n");
    return 0;
}

int isBlocked(char *IPaddress, peerSideList **head)
{
    peerSideList *temp = *head;
    while (temp != NULL)
    {
        printf("string 1: %s string 2: %s\n", temp->IPaddress, IPaddress);
        printf("comparison %d\n", strcmp(temp->IPaddress, IPaddress));
        if (strcmp(temp->IPaddress, IPaddress) == 0)
        {
            if (temp->isBlocked == 1)
                return 1;
        }
        temp = temp->next;
    }
    return 0;
}

void updateBlockStat(int block, char *IPaddress, peerSideList **head)
{
    peerSideList *temp = *head;
    while (temp != NULL)
    {
        printf("string 1: %s string 2: %s\n", temp->IPaddress, IPaddress);
        printf("comparison %d\n", strcmp(temp->IPaddress, IPaddress));
        if (strcmp(temp->IPaddress, IPaddress) == 0)
        {
            temp->isBlocked = block;
        }
        temp = temp->next;
    }
}

int existInServerList(char *IPaddress, clientList **head)
{
    printf("\nInside is client list function\n");
    clientList *temp = *head;
    while (temp != NULL)
    {
        printf("string 1: %s string 2: %s\n", temp->IPaddress, IPaddress);
        printf("comparison %d\n", strcmp(temp->IPaddress, IPaddress));
        if (strcmp(temp->IPaddress, IPaddress) == 0)
        {
            printf("I am returned 1\n");
            return 1;
        }
        temp = temp->next;
    }
    printf("I am returned 0\n");
    return 0;
}

// 	1. https://github.com/Vaishnavi3799/Text_Chat_Application
// 	2. https://github.com/shaduk/Text-Chat-Application-over-TCP