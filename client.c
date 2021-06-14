#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h> 
#define SIZE 1024
#define FILECHUNK 4096
#define DEBUG 1
#define PORT 8000

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

int getFileSize(FILE *fp) {
    int fSize;
    // Count seq file length
    fseek(fp, 0, SEEK_END);
    fSize = ftell(fp);
    rewind(fp);
    return fSize;
}

int SocketReceive(int hSocket, char *Rsp, int RvcSize)
{
    int shortRetval = -1;
    struct timeval tv;
    tv.tv_sec = 20; /* 20 Secs Timeout */
    tv.tv_usec = 0;
    if (setsockopt(hSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)
    {
        perror("Time out...\n");
        exit(EXIT_FAILURE);
    }
    shortRetval = recv(hSocket, Rsp, RvcSize, 0);
    return shortRetval;
}

int main()
{
    char *ip = "127.0.0.1";
    int e;

    int sockfd;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Server socket created successfully.\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = PORT;
    server_addr.sin_addr.s_addr = inet_addr(ip);

    e = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (e == -1)
    {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Connected to Server.\n");

    FILE *fref, *fseq;
    char op, message[SIZE] = {0};
    char *file = NULL;
    int fSize;

    while((op = menu()) != '2') {
        if(op == '0') {
            // Open seq file
            if((fseq = fopen("seq.txt", "r")) == NULL) {
                perror("Error opening the file");
                exit(EXIT_FAILURE);
            }

            // Count seq file length
            fSize = getFileSize(fseq);

            // Allocate buffer to store file
            file = malloc(sizeof(char) * (fSize + 2)); // Add 2 (1 for option, 1 for null)
            size_t newLen = fread(file, sizeof(char), fSize, fseq);
            if ( ferror( fseq ) != 0 ) {
                perror("Error reading file");
                exit(EXIT_FAILURE);
            } else {
                file[newLen++] = '0'; // Adds option
                file[newLen++] = '\0'; // Add NULL terminator
            }

            // Send length to server
            sprintf(message, "%zu", newLen);
            printf("%s\n", message);
            if(send(sockfd, message, SIZE, 0) == -1) {
                perror("Error sending message length");
                exit(EXIT_FAILURE);
            }

            printf("%zu\n", newLen);
            printf("Message tail: %s\n", file + newLen - 11); // Show last 10 chars of seq
            // Send File
            void *p = file;
            int bytes_written = 0;
            while (1)
            {
                bytes_written += write(sockfd, p, SIZE);
                if(bytes_written >= fSize) {
                    break;
                }
                p = file + bytes_written;
            }

            // (Optional) Receive OK message from server 
            char status[3] = {0};
            SocketReceive(sockfd, status, sizeof(status));
            if(strcmp(status, "OK") == 0) printf("Upload Success\n");
            fclose(fseq);
            free(file);
        } else if(op == '1') {
            // strcpy(message, "1");
        } else {
            printf("Please enter a valid option");
            continue;
        }
        // close(fd);
    }

    close(sockfd);
    printf("[+]Connection closed.\n");
    return 0;
}