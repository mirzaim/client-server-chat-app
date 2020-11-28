
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define MAX_CLIENT 64


int serverPort;

bool flag = true;

char buffer[256];

struct client{
    int fd;
    char username[64];
};

struct group{
    char groupID[64];
    int nMember;
    struct client members[64];
};

int serverSocket;

struct client clients[MAX_CLIENT];
int nClient = 0;

struct group groups[8];
int nGroup = 0;

int findClientByUsername(char *username){
    for(int i = 0; i < nClient; i++)
        if(strcmp(clients[i].username, username) == 0)
            return i;
    return -1;
}

void addClient(int fd, char *username){
    clients[nClient].fd = fd;
    strcpy(clients[nClient].username, username);
    nClient++;
}

void closeAllClients(){
    strcpy(buffer, "close");
    for (int i = 0; i < nClient; i++){
        write(clients[i].fd, buffer, sizeof(buffer));
        close(clients[i].fd);
    }
    nClient = 0;
}


int findGroupByGroupID(char *groupID){
    for (int i = 0; i < nGroup; i++)
        if(strcmp(groups[i].groupID, groupID) == 0)
            return i;
    return -1;
}

int findMemberByUsername(struct group *group, char *username){
    for(int i = 0; i < group->nMember; i++)
        if (strcmp(group->members[i].username, username) == 0)
            return i;
    return -1;
}

void closeC(char *username){
    int i = findClientByUsername(username);
    close(clients[i++].fd);
    printf("%s removed.\n", username);
    for (; i < nClient; i++)
        clients[i-1] = clients[i];
    nClient--;
}


void joinC(char *username, char *groupID){
    struct group *group = &groups[findGroupByGroupID(groupID)];
    if (findMemberByUsername(group, username) < 0)
        group->members[group->nMember++] = clients[findClientByUsername(username)];
}

void leaveC(char *username, char *groupID){
    struct group *group = &groups[findGroupByGroupID(groupID)];
    int i = 0;
    for (; (i < group->nMember) && (strcmp(group->members[i].username, username)); i++);
    i++;
    for (; i < group->nMember; i++)
        group->members[i-1] = group->members[i];
    group->nMember--;
}

void createC(char *username, char *groupID){
    if (findGroupByGroupID(groupID) < 0){
        strcpy(groups[nGroup].groupID, groupID);
        groups[nGroup].nMember = 0;
        nGroup++;
        printf("group %s created by %s.\n", groupID, username);
        joinC(username, groupID);
    }
}

void sendC(char *username, char *groupID, char *message){
    struct group *group = &groups[findGroupByGroupID(groupID)];
    strcpy(buffer, groupID);
    strcat(buffer, "->");
    strcat(buffer, username);
    strcat(buffer, ": ");
    strcat(buffer, message);

    if (findMemberByUsername(group, username) >= 0){
        printf("%s\n", buffer);
        for (int i = 0; i < group->nMember; i++)
            write(group->members[i].fd, buffer, sizeof(buffer));
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
    while (flag){
        FD_ZERO(&readfds);

        FD_SET(serverSocket, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        max_fd = (serverSocket > STDIN_FILENO)? serverSocket : STDIN_FILENO;

        for (int i = 0; i < nClient; i++){
            FD_SET(clients[i].fd, &readfds);
            if (clients[i].fd > max_fd)
                max_fd = clients[i].fd;
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
            if (findClientByUsername(buffer) < 0){
                addClient(newClient, buffer);
                strcpy(buffer, "connected\n");
                printf("New client\n");
            } else
                strcpy(buffer, "username is taken\n");

            write(newClient, buffer, sizeof(buffer));
        }

        for (int i = 0; i < nClient; i++)
            if (FD_ISSET(clients[i].fd, &readfds)){
                read(clients[i].fd, buffer, sizeof(buffer));
                parse(clients[i].username, buffer);
            }
    }

    closeAllClients();
    close(serverSocket);
    return 0;
}
