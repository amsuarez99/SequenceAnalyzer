#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <fcntl.h>
#define SIZE 1024
#define FILECHUNK 4096
#define DEBUG 1
#define PORT 8000

enum _OPTIONS {
    SEQ,
    REF,
    EXIT
};

void writeFile(int sockfd, int fd) {
    char buff[FILECHUNK];
    int cRead;
    while(1) {
        cRead = read(sockfd, buff, sizeof(buff));
        if(cRead <= 0) {
            break;
            return;
        }
        write(fd, buff, cRead);
        bzero(buff, FILECHUNK);
    }
    return;
}


void readOption(int sockfd) {
    char buff[SIZE] = {0};
    read(sockfd, buff, SIZE);
    printf("%s", buff);
}

void setup(int sockfd) {
    char *fName = malloc(sizeof(char) * SIZE);
    int fd, cRead;
    char clientMsg[SIZE] = {0};

    while ((cRead = read(sockfd, clientMsg, sizeof(clientMsg))) > 0) {
        if(strcmp(clientMsg, "0") == 0) {
            fName = "seq.txt";
            if((fd = open(fName, O_WRONLY | O_CREAT)) == -1) {
                perror("[-]Error opening file");
                exit(EXIT_FAILURE);
            }
            writeFile(sockfd, fd);
            close(fd);
        } 
        else if(strcmp(clientMsg, "1") == 0) {
            fName = "ref.txt";
            if((fd = open(fName, O_WRONLY | O_CREAT)) == -1) {
                perror("[-]Error opening file");
                exit(EXIT_FAILURE);
            }
            writeFile(sockfd, fd);
            close(fd);
        }
        bzero(clientMsg, SIZE);
        bzero(fName, SIZE);
    }

    // while(recv(sockfd, buffer, SIZE, 0)) {
    //     // printf("%s\n", buffer);
    //     switch (option)
    //     {
    //         case SEQ:
    //         {
    //             printf("INTRODUCING SEQ...\n");
    //             fName = "seq.txt";
    //             if((fd = open(fName, O_WRONLY | O_CREAT)) == -1) {
    //                 perror("[-]Error opening file");
    //                 exit(EXIT_FAILURE);
    //             }
    //             writeFile(sockfd, fd);
    //             close(fd);
    //             break;
    //         }
    //         case REF:
    //         {
    //             printf("INTRODUCING REF...\n");
    //             fName = "ref.txt";
    //             if((fd = open(fName, O_WRONLY | O_CREAT)) == -1) {
    //                 perror("[-]Error opening file");
    //                 exit(EXIT_FAILURE);
    //             }
    //             writeFile(sockfd, fd);
    //             close(fd);
    //             break;
    //         }
    //         case EXIT: {
    //             break;
    //             return;
    //         }
    //     }
    //     bzero(buffer, SIZE);
    // }
}

// int main()
// {
//     char *ip = "127.0.0.1";
//     int port = 8080;
//     int e;

//     int serverfd, clientfd, readLength;
//     struct sockaddr_in server_addr, new_addr;
//     socklen_t addr_size;

//     char clientMsg[SIZE] = {0};
//     char serverMsg[SIZE] = {0};

//     // Create Scoket File Descriptor
//     serverfd = socket(AF_INET, SOCK_STREAM, 0);
//     if (serverfd < 0)
//     {
//         perror("[-]Error in socket");
//         exit(1);
//     }
//     printf("[+]Server socket created successfully.\n");

//     //Forcefully attaching socket to the port 8080
//     int opt = 1;
//     if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR,
//                    &opt, sizeof(opt)))
//     {
//         syslog(LOG_NOTICE, "[-]Error setting socket options");
//         perror("[-]Error setting socket options");
//         exit(EXIT_FAILURE);
//     }

//     server_addr.sin_family = AF_INET;
//     server_addr.sin_port = port;
//     server_addr.sin_addr.s_addr = inet_addr(ip);

//     // Bind Adress to Socket
//     if (bind(serverfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
//     {
//         perror("[-]Error in bind");
//         exit(1);
//     }
//     printf("[+]Binding successfull.\n");

//     // Listen
//     if (listen(serverfd, 3) == 0)
//     {
//         printf("[+]Listening....\n");
//     }
//     else
//     {
//         perror("[-]Error in listening");
//         exit(1);
//     }

//     addr_size = sizeof(new_addr);
//     while((clientfd = accept(serverfd, (struct sockaddr *)&new_addr, &addr_size)) > 0) {
//         printf("[+]Connection accepted.\n");
//         // syslog(LOG_NOTICE, "Connection Accepted");
//         printf("Hello world");
//         // fflush(stdout);

//         // Receives length of next message
//         while((readLength = recv(clientfd, clientMsg, SIZE, 0)) > 0) {
//             printf("Hello world");
//             int nextMsgLength = atoi(clientMsg);
//             bzero(clientMsg, SIZE);

//             // Allocates mem based on next msg len
//             char *buffer = (char*) malloc((sizeof(char) * nextMsgLength) + 2);
//             bzero(buffer, nextMsgLength + 2);

//             // Receives actual message
//             recv(clientfd, buffer, nextMsgLength, 0);
//             bzero(clientMsg, SIZE);

//             // Print message tail
//             printf("%s", buffer + nextMsgLength - 11);
//         }

//         close(clientfd);
//         // setup(clientfd);

//         // int fd = open("ref2.txt", O_WRONLY | O_CREAT);
//         // writeFile(clientfd, fd);
//         // write_file(clientfd);
//         // printf("[+]Data written in the file successfully.\n");
//         // printf("[+]Connection terminated.\n");
//         // fflush(stdout);
//     }
//     return 0;
// }

int main()
{
    char *ip = "127.0.0.1";
    int serverfd, clientfd, readLength;
    socklen_t addr_size;
    struct sockaddr_in server, client;
    char clientMsg[SIZE] = {0};
    char serverMsg[SIZE] = {0};
    char *buffer = NULL;

    // Create socket fd
    serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverfd == -1)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }
    printf("Created Socket Successfully\n");

    //Forcefully attaching socket to the port 8080
    int opt = 1;
    if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR,
                   &opt, sizeof(opt)))
    {
        perror("[-]Error setting socket options");
        exit(EXIT_FAILURE);
    }

    server.sin_family = AF_INET;
    server.sin_port = PORT;
    server.sin_addr.s_addr = inet_addr(ip);

    // Bind
    if (bind(serverfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Error in bind");
        exit(EXIT_FAILURE);
    }
    printf("Bind Successfull\n");

    // Listen
    if (listen(serverfd, 3) == 0)
    {
        printf("[+]Listening....\n");
    }
    else
    {
        perror("[-]Error in listening");
        exit(EXIT_FAILURE);
    }
    addr_size = sizeof(struct sockaddr_in);
    while ((clientfd = accept(serverfd, (struct sockaddr *)&client, (socklen_t *)&addr_size)))
    {
        printf("[+]Connection accepted.\n");
        // syslog(LOG_NOTICE, "Connection Accepted");
        // printf("Hello world");
        fflush(stdout);

        // Receives length of next message
        while ((readLength = recv(clientfd, clientMsg, sizeof(clientMsg), 0)) <= SIZE)
        {
            int nextMsgLength = atoi(clientMsg);
            printf("%d\n", nextMsgLength);

            // Allocates mem based on next msg len
            buffer = (char *) malloc(sizeof(char) * nextMsgLength);
            bzero(buffer, nextMsgLength);

            // Receive message in chunks
            int n = 0;
            int xRead;
            while(n < nextMsgLength) {
                xRead = read(clientfd, clientMsg, SIZE);
                memcpy(buffer + n, clientMsg, xRead);
                bzero(clientMsg, SIZE);
                n += xRead;
            }
            // Print message tail
            printf("Message tail: %s\n", buffer + strlen(buffer) - 10);
            printf("%c\n", *(buffer + strlen(buffer) - 1));

            send(clientfd, "OK", 3, 0);
            bzero(clientMsg, SIZE);
            free(buffer);
        }
        fflush(stdout);
        close(clientfd);
    }
    return 0;
}