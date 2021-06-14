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

struct _Sequence {
    char *sequenceString;
    int found;
    int position;
};

int main()
{
    char *ip = "127.0.0.1";
    int serverfd, clientfd, readLength;
    socklen_t addr_size;
    struct sockaddr_in server, client;
    char clientMsg[SIZE] = {0};
    char serverMsg[SIZE] = {0};
    char *buffer = NULL;


    char *sequence = NULL, *reference = NULL;
    char option;
    int uploadedSeq = 0, uploadedRef = 0;

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
            // #if DEBUG == 1
            // printf("Message tail: %s\n", buffer + strlen(buffer) - 10);
            // printf("%c\n", *(buffer + strlen(buffer) - 1));
            // #endif

            // View Option
            option = *(buffer + strlen(buffer) - 1);
            *(buffer + strlen(buffer) - 1) = '\0';
            if(option == '0') {
                if(uploadedSeq) {
                    free(sequence);
                }
                uploadedSeq = 1;
                sequence = strdup(buffer);
                printf("Received sequence: %s\n", sequence + strlen(sequence) - 10);
            } else if(option == '1') {
                if(uploadedRef) {
                    free(reference);
                }
                uploadedRef = 1;
                reference = strdup(buffer);
                printf("Received ref: %s\n", reference + strlen(reference) - 10);
            } else if(option == 'R') {
                if(uploadedSeq && uploadedRef) {
                    printf("Client wants results...\n");
                    strcpy(clientMsg, "====RESULTS====\nPercentage covered: 20%\nSequences used:\nAGAGAGGATT\nAGGGAGGAGT\n");
                } else {
                    printf("Can't view results without uploading ref or seq\n");
                    strcpy(clientMsg, "Can't view results without uploading ref or seq\n");
                }
                send(clientfd, clientMsg, SIZE, 0);
            }

            // SEND OPERATION COMPLETE
            send(clientfd, "OK", 3, 0);
            bzero(clientMsg, SIZE);
            free(buffer);
            fflush(stdout);
        }
        close(clientfd);
    }
    return 0;
}