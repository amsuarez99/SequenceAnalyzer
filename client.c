#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h> 
#define SIZE 1024

void sendFile(int sockfd) {
    int fd = open("reference.txt", O_RDONLY);
    char buff[4096];
    int cRead;
    while((cRead = read(fd, buff, sizeof(buff))) > 0) {
        write(sockfd, buff, cRead);
        bzero(buff, 4096);
    }
}

int main()
{
    char *ip = "127.0.0.1";
    int port = 8080;
    int e;

    int sockfd;
    struct sockaddr_in server_addr;
    FILE *fp;
    char *filename = "reference.txt";

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Server socket created successfully.\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr = inet_addr(ip);

    e = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (e == -1)
    {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Connected to Server.\n");

    // send_file(fp, sockfd);
    sendFile(sockfd);
    printf("[+]File data sent successfully.\n");

    return 0;
}