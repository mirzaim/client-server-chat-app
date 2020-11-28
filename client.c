
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>


bool flag = true;

char username[64];
char serverHostName[64];
int serverPort;

int clientSocket;
char buffer[256];



int main(int argc, char *argv[]){
    if(argc < 4)
        return -1;

    strcpy(serverHostName, argv[1]);
    serverPort = atoi(argv[2]);
    strcpy(username, argv[3]);

    struct sockaddr_in serv_addr;
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -2;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverHostName, &serv_addr.sin_addr) <= 0)
        return -3;


    if (connect(clientSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        return -4;

    // send username as first message
    strcpy(buffer, username);
    write(clientSocket, buffer, sizeof(buffer));
    read(clientSocket, buffer, sizeof(buffer));
    printf("%s", buffer);
    if(strcmp(buffer, "connected\n") != 0)
        return -5;

    fd_set readfds;
    int max_fd;
    while (flag){
        FD_ZERO(&readfds);

        FD_SET(clientSocket, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        max_fd = (clientSocket > STDIN_FILENO)? clientSocket : STDIN_FILENO;

        select(max_fd + 1, &readfds, NULL, NULL, NULL);

        if(FD_ISSET(STDIN_FILENO, &readfds)){
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\r\n")] = '\0';

            if (strcmp(buffer, "quit") == 0){
                flag = false;
                strcpy(buffer, "close");
            }
            
            write(clientSocket, buffer, sizeof(buffer));
        }

        if (FD_ISSET(clientSocket, &readfds)){
            read(clientSocket, buffer, sizeof(buffer));
            if (strcmp(buffer, "close") == 0)
                flag = false;
            else
                printf("%s\n", buffer);
        }
    }

    close(clientSocket);
    return 0;
}


