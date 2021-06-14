#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h> 
#define SIZE 1024
#define FILECHUNK 4096
#define DEBUG 1

void *getInput() {
    char *input = (char *) malloc(sizeof(char) * SIZE);
    memset(input, '\0', SIZE);
    scanf("%s", input);
    #if DEBUG == 1
    printf("You entered: %s\n", input);
    #endif
    fflush(stdin);
    return input;
}

void sendFile(int sockfd, int fd) {
    char buff[FILECHUNK];
    int cRead;
    while((cRead = read(fd, buff, sizeof(buff))) > 0) {
        write(sockfd, buff, cRead);
        bzero(buff, FILECHUNK);
    }
}

void sendFile(int sockfd, int fd) {
    char buff[FILECHUNK];
    int cRead;
    while()
}

char menu() {
    char op;
    printf("===MAIN MENU===\n");
    printf("0) Upload Sequence\n");
    printf("1) Upload Reference\n");
    printf("2) Exit\n");
    printf("Please enter an option\n");
    scanf("%c", &op);
    fflush(stdin);
    return op;
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

    char op;
    int fd;
    char message[SIZE] = {0};
    while((op = menu()) != '2') {
        if(op == '0') {
            strcpy(message, "0");

        } else if(op == '1') {
            strcpy(message, "1");
        } else {
            printf("Please enter a valid option");
            continue;
        }
        // write(sockfd, message, sizeof(message));
        if((fd = open("reference.txt", O_RDONLY)) == -1) {
            perror("[-]Error opening file");
            exit(EXIT_FAILURE);
        }
        sendFile(sockfd, fd);
        printf("File copied successfully\n");
        close(fd);
    }

    close(sockfd);
    printf("[+]Connection closed.\n");

    return 0;
}