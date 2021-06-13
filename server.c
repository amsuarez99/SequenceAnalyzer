#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <fcntl.h>
#define SIZE 1024
#define DEBUG 1

void writeFile(int sockfd) {
    int fd = open("hello3.txt", O_WRONLY | O_CREAT);
    char buff[4096];
    int cRead;
    while((cRead = read(sockfd, buff, sizeof(buff))) > 0) {
        write(fd, buff, cRead);
        bzero(buff, 4096);
    }
}

int main()
{
    char *ip = "127.0.0.1";
    int port = 8080;
    int e;

    int serverfd, clientfd;
    struct sockaddr_in server_addr, new_addr;
    socklen_t addr_size;
    char buffer[SIZE];

    // Create Scoket File Descriptor
    serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverfd < 0)
    {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Server socket created successfully.\n");

    //Forcefully attaching socket to the port 8080
    int opt = 1;
    if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR,
                   &opt, sizeof(opt)))
    {
        syslog(LOG_NOTICE, "[-]Error setting socket options");
        perror("[-]Error setting socket options");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr = inet_addr(ip);

    // Bind Adress to Socket
    if (bind(serverfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("[-]Error in bind");
        exit(1);
    }
    printf("[+]Binding successfull.\n");

    while(1) {
        // Listen
        if (listen(serverfd, 1) == 0)
        {
            printf("[+]Listening....\n");
        }
        else
        {
            perror("[-]Error in listening");
            exit(1);
        }

        addr_size = sizeof(new_addr);
        if((clientfd = accept(serverfd, (struct sockaddr *)&new_addr, &addr_size)) < 0) {
            perror("[-]Error in accept");
            exit(EXIT_FAILURE);
        }
        #if DEBUG == 1
        printf("[+]Connection accepted.\n");
        #endif
        syslog(LOG_NOTICE, "Connection Accepted");

        writeFile(clientfd);
        // write_file(clientfd);
        printf("[+]Data written in the file successfully.\n");
        printf("[+]Connection terminated.\n");
    }
    return 0;
}