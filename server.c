
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>

#include "list.h"

#define MAX_CLIENT 64


int serverPort;

bool flag = true;

char buffer[256];

struct client{
    int fd;
    char username[64];
    struct client *next;
    struct client *prev;
};


struct group{
    char groupID[64];
    int nMember;
    struct client *members[32];
    struct group *next;
    struct group *prev;
};


int serverSocket;

struct client clientHead = initHead(clientHead);

struct group groupHead = initHead(groupHead);

struct client *findClientByUsername(char *username){
    struct client *client;
    foreachOnNode(client, &clientHead)
        if(!strcmp(client->username, username))
            return client;
    return NULL;
}

void addClient(int fd, char *username){
    struct client *newClient = (struct client *)malloc(sizeof(struct client));
    newClient->fd = fd;
    strcpy(newClient->username, username);    
    addNode(newClient, &clientHead);
}

void closeAllClients(){
    strcpy(buffer, "close");
    struct client *client;
    foreachOnNode(client, &clientHead){
        write(client->fd, buffer, sizeof(buffer));
        close(client->fd);
        free(client);
    }
}


struct group *findGroupByGroupID(char *groupID){
    struct group *group;
    foreachOnNode(group, &groupHead){
        if(!strcmp(group->groupID, groupID))
            return group;
    }
    return NULL;
}

struct client *findMemberByUsername(struct group *group, char *username){
    for(int i = 0; i < group->nMember; i++)
        if (!strcmp(group->members[i]->username, username))
            return group->members[i];
    return NULL;
}

void closeC(char *username){
    struct client *client = findClientByUsername(username);
    close(client->fd);
    printf("%s removed.\n", username);
    deleteNode(client, clientHead);
}


void joinC(char *username, char *groupID){
    struct group *group = findGroupByGroupID(groupID);
    if (!findMemberByUsername(group, username))
        group->members[group->nMember++] = findClientByUsername(username);
}

void leaveC(char *username, char *groupID){
    struct group *group = findGroupByGroupID(groupID);
    int i = 0;
    for (; (i < group->nMember) && (strcmp(group->members[i]->username, username)); i++);
    i++;
    for (; i < group->nMember; i++)
        group->members[i-1] = group->members[i];
    group->nMember--;
}

void createC(char *username, char *groupID){
    if (!findGroupByGroupID(groupID)){
        struct group *group = (struct group *)malloc(sizeof(struct group));
        strcpy(group->groupID, groupID);
        group->nMember = 0;
        addNode(group, &groupHead);
        printf("group %s created by %s.\n", groupID, username);
        joinC(username, groupID);
    }
}

void sendC(char *username, char *groupID, char *message){
    struct group *group = findGroupByGroupID(groupID);
    strcpy(buffer, groupID);
    strcat(buffer, "->");
    strcat(buffer, username);
    strcat(buffer, ": ");
    strcat(buffer, message);

    if (findMemberByUsername(group, username)){
        printf("%s\n", buffer);
        for (int i = 0; i < group->nMember; i++)
            write(group->members[i]->fd, buffer, sizeof(buffer));
    }
}

int parse(char *username, char *input){
    char command[64];
    char groupID[64];
    char message[128];

    char *ptr;
    strcpy(command, (ptr = strtok(input, " "))? ptr : ""); 
    strcpy(groupID, (ptr = strtok(NULL, " "))? ptr : "");
    strcpy(message, (ptr = strtok(NULL, ""))? ptr : "");


    if(!strcmp(command, "close"))
        closeC(username);
    else if (!strcmp(command, "create"))
        createC(username, groupID);
    else if (!strcmp(command, "join"))
        joinC(username, groupID);
    else if (!strcmp(command, "leave"))
        leaveC(username, groupID);
    else if (!strcmp(command, "send"))
        sendC(username, groupID, message);
    else
        return -1;

    return 1;
}

int main(int argc, char *argv[]){
    if (argc < 2)
        return -1;

    serverPort = atoi(argv[1]);

    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -2;

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,  &opt, sizeof(opt)) < 0)
        return -3;

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(serverPort);
    int serv_addr_len = sizeof(serv_addr);
    if (bind(serverSocket, (struct sockaddr *)&serv_addr, serv_addr_len) < 0)
        return -4;

    if (listen(serverSocket, 32) < 0)
        return -5;

    printf("Server started.\n");

    fd_set readfds;
    int max_fd;
    struct client *client;
    while (flag){
        FD_ZERO(&readfds);

        FD_SET(serverSocket, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        max_fd = (serverSocket > STDIN_FILENO)? serverSocket : STDIN_FILENO;


        foreachOnNode(client, &clientHead){
            FD_SET(client->fd, &readfds);
            if (client->fd > max_fd)
                max_fd = client->fd;
        }

        select(max_fd + 1, &readfds, NULL, NULL, NULL);

        if(FD_ISSET(STDIN_FILENO, &readfds)){
            fgets(buffer, sizeof(buffer), stdin);
            if (strcmp(buffer, "quit\n") == 0)
                flag = false;
        }

        if (FD_ISSET(serverSocket, &readfds)){
            int newClient;
            if ((newClient = accept(serverSocket, (struct sockaddr *)&serv_addr, (socklen_t *)&serv_addr_len)) < 0)
                return -6;
            read(newClient, buffer, sizeof(buffer));
            if (!findClientByUsername(buffer)){
                addClient(newClient, buffer);
                strcpy(buffer, "connected\n");
                printf("New client\n");
            } else
                strcpy(buffer, "username is taken\n");

            write(newClient, buffer, sizeof(buffer));
        }

        foreachOnNode(client, &clientHead)
            if (FD_ISSET(client->fd, &readfds)){
                read(client->fd, buffer, sizeof(buffer));
                parse(client->username, buffer);
            }
    }

    closeAllClients();
    close(serverSocket);
    return 0;
}
