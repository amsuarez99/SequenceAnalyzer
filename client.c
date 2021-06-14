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
    scanf(" %s", input);
    #if DEBUG == 1
    printf("You entered: %s\n", input);
    #endif
    fflush(stdin);
    return input;
}

char menu() {
    char op;
    printf("===MAIN MENU===\n");
    printf("0) Upload Sequence\n");
    printf("1) Upload Reference\n");
    printf("2) View Results\n");
    printf("3) Exit\n");
    printf("Please enter an option\n");
    scanf("%c", &op);
    fflush(stdin);
    return op;
}

int getFileSize(FILE *fp) {
    int fSize;
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

    FILE *fp;
    char op, message[SIZE] = {0}, status[3] = {0};
    char *file = NULL, *fName;
    int fSize;

    while((op = menu()) != '3') {
        if(op == '0' || op == '1') {
            printf("Enter file name: ");
            fName = getInput();
            if((fp = fopen(fName, "r")) == NULL) {
                perror("Error opening the file");
                exit(EXIT_FAILURE);
            }
            if(op == '0') {
                // Verify Seq
            } else if(op == '1') {
                // Verify Ref
            }

            // Count file length
            fSize = getFileSize(fp);

            // Allocate buffer to store file
            file = malloc(sizeof(char) * (fSize + 2)); // Add 2 (1 for option, 1 for null)
            size_t newLen = fread(file, sizeof(char), fSize, fp);
            if ( ferror( fp ) != 0 ) {
                perror("Error reading file");
                exit(EXIT_FAILURE);
            } else {
                file[newLen++] = op; // Adds option
                file[newLen++] = '\0'; // Add NULL terminator
            }

            // Send length to server
            sprintf(message, "%zu", newLen);
            if(send(sockfd, message, SIZE, 0) == -1) {
                perror("Error sending message length");
                exit(EXIT_FAILURE);
            }

            printf("Length sent: %zu\n", newLen);

            // Send File in chunks
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
            printf("Message sent: %s (SHOWING TAIL)\n", file + newLen - 11); // Show last 10 chars of seq

            fclose(fp);
            free(file);
            free(fName);
        }   else if (op == '2') {
            // Want to send 2 characters ['R','\0']
            memcpy(message, "2", 2);
            send(sockfd, message, SIZE, 0);
            send(sockfd, "R", 2, 0);

            bzero(message, SIZE);
            // RECEIVE MESSAGE FROM RESULT OP
            SocketReceive(sockfd, message, SIZE);
            printf("%s", message);
        } else  {
            printf("Please enter a valid option");
            continue;
        }

        // (Optional) Receive OK message from server 
        SocketReceive(sockfd, status, sizeof(status));
        if(strcmp(status, "OK") == 0) printf("Operation Completed\n");

        // RESET
        bzero(status, sizeof(status));
        bzero(message, SIZE);
    }

    close(sockfd);
    printf("[+]Connection terminated.\n");
    return 0;
}