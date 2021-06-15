#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <fcntl.h>
#include <pthread.h>
#define SIZE 1024
#define FILECHUNK 4096
#define DEBUG 1
#define PORT 8000
#define NTHREADS 10

pthread_mutex_t lock;

struct thread_attributes
{
    size_t thread_num;
    size_t nThreads;
    size_t nSequences;
    int *nFound;
    char *ref;
    struct _Sequence *sequences;
};

struct _Sequence
{
    char *sequenceString;
    size_t size;
    int position;
};

void *findSeq(void *x)
{
    struct thread_attributes arguments;
    arguments = *((struct thread_attributes *)x);
    char *offset;
    int index;
    int load = arguments.nSequences/arguments.nThreads;
    // Add remaining load to the last thread
    if(arguments.thread_num == (arguments.nThreads - 1)) {
        load += arguments.nSequences - (load * arguments.nThreads);
    }
    for(int j = 0; j < load; j++) {
        index = (arguments.thread_num * arguments.nSequences/arguments.nThreads) + j;
        if((offset = strstr(arguments.ref, arguments.sequences[index].sequenceString)) != NULL) {
            arguments.sequences[index].position = offset - arguments.ref;
            printf("Sequence %d: %*.*s (HEAD) found at character %d\n", 
                    index,
                    1, 10, 
                    arguments.sequences[index].sequenceString,
                    arguments.sequences[index].position);
            pthread_mutex_lock(&lock);
            *arguments.nFound = *arguments.nFound + 1;
            pthread_mutex_unlock(&lock);
        } else {
            printf("Sequence %d: %*.*s (HEAD) not found\n",
                    index,
                    1, 10, 
                    arguments.sequences[index].sequenceString);
        }
    }
    fflush(stdout);
    return NULL;
}

void copy(struct _Sequence *seqs, size_t nSeqs, size_t nFound) {

}

void printResults(struct _Sequence *seqs, size_t nSeqs, size_t nFound)
{
    printf("%zu sequences were found out of the %zu provided\n", nFound, nSeqs);
    printf("Sequences: (ONLY SHOWING TAIL)\n");
    for (size_t i = 0; i < nSeqs; i++)
    {
        if (seqs[i].position != -1)
            printf("(Sequence #%zu) %s found at character %d\n",
                   i,
                   seqs[i].sequenceString + seqs[i].size - 10,
                   seqs[i].position);
        else
            printf("(Sequence #%zu) %s not found\n",
                   i,
                   seqs[i].sequenceString + seqs[i].size - 10);
    }
}

int separateSequences(char *seq, struct _Sequence *seqs)
{
    const char delim[2] = "\n";
    char *token;
    int pos = 0;

    // get the first token
    token = strtok(seq, delim);

    // walk through other tokens
    while (token != NULL)
    {
        // printf("%s\n%lu\n", token, strlen(token) - 1);
        token[strlen(token) - 1] = '\0';
        seqs[pos].size = strlen(token) - 1;
        seqs[pos].sequenceString = strdup(token);
        seqs[pos].position = -1;
        token = strtok(NULL, delim);
        pos++;
    }
    return pos;
}

int main()
{
    char *ip = "127.0.0.1";
    int serverfd, clientfd, readLength;
    socklen_t addr_size;
    struct sockaddr_in server, client;

    char clientMsg[SIZE] = {0};
    char *buffer = NULL, *sequence = NULL, *reference = NULL;
    char *offset;

    // struct _Sequence **sequences;
    struct _Sequence sequences[1024];
    size_t nSequences;

    char option;
    int uploadedSeq = 0, uploadedRef = 0, nFound = 0;

    // Create socket fd
    if ((serverfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[-]Error creating socket");
        exit(EXIT_FAILURE);
    }
    printf("[+]Socket created successfully.\n");

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
        perror("[-]Error in bind");
        exit(EXIT_FAILURE);
    }
    printf("[+]Binding successfull.\n");

    while (1)
    {
        // Listen
        if (listen(serverfd, 1) != 0)
        {
            perror("[-]Error in listen");
            exit(EXIT_FAILURE);
        }
        printf("[+]Listening....\n");

        addr_size = sizeof(struct sockaddr_in);
        if ((clientfd = accept(serverfd, (struct sockaddr *)&client, (socklen_t *)&addr_size)) < 0)
        {
            perror("[-]Error in accept");
            exit(EXIT_FAILURE);
        }
        printf("[+]Connection accepted.\n");

        // Receives length of next message
        while ((readLength = recv(clientfd, clientMsg, sizeof(clientMsg), 0)))
        {
            int nextMsgLength = atoi(clientMsg);
            printf("[+]Message length: %d\n", nextMsgLength);

            // Allocates mem based on next msg len
            buffer = (char *)malloc(sizeof(char) * nextMsgLength);
            bzero(buffer, nextMsgLength);

            // Receive message in chunks
            int n = 0;
            int xRead;
            while (n < nextMsgLength)
            {
                xRead = read(clientfd, clientMsg, SIZE);
                memcpy(buffer + n, clientMsg, xRead);
                bzero(clientMsg, SIZE);
                n += xRead;
            }

            // View Option
            option = *(buffer + strlen(buffer) - 1);
            *(buffer + strlen(buffer) - 1) = '\0';
            if (option == '0')
            {
                if (uploadedSeq)
                {
                    // Reset sequences
                    bzero(sequences, sizeof(sequences));
                    free(sequence);
                }
                uploadedSeq = 1;

                // Copy Buffer to sequence
                sequence = strdup(buffer);
                printf("[+]Received sequence: %s (SHOWING TAIL)\n", sequence + strlen(sequence) - 10);

                // Separate Sequences to array
                nSequences = separateSequences(sequence, sequences);
            }
            else if (option == '1')
            {
                if (uploadedRef)
                {
                    free(reference);
                }
                uploadedRef = 1;
                reference = strdup(buffer);
                printf("[+]Received ref: %s (SHOWING TAIL)\n", reference + strlen(reference) - 10);
            }
            else if (option == 'R')
            {
                if (uploadedSeq && uploadedRef)
                {
                    nFound = 0;
                    printf("[+]Client wants results...\n");
                    strcpy(clientMsg, "====RESULTS====\nPercentage covered: 20%\nSequences used:\nAGAGAGGATT\nAGGGAGGAGT\n");
                    const size_t nThreads = nSequences < NTHREADS ? nSequences : NTHREADS; // Get min
                    pthread_t threads[nThreads];
                    struct thread_attributes thread_args[nThreads];

                    if (pthread_mutex_init(&lock, NULL) != 0) {
                        perror("Mutex init failed\n");
                        exit(EXIT_FAILURE);
                    }

                    // Spawn a thread for each row
                    size_t load = nSequences/nThreads;
                    for (size_t i = 0; i < nThreads; ++i)
                    {
                        struct thread_attributes a = {i, nThreads, nSequences, &nFound, reference, sequences};
                        thread_args[i] = a;
                        pthread_create(&threads[i], NULL, findSeq, (void *)&thread_args[i]);
                    }

                    /* wait for threads to finish */
                    for (size_t i = 0; i < nThreads; ++i)
                    {
                        pthread_join(threads[i], NULL);
                    }
                    pthread_mutex_destroy(&lock);

                    printf("Found sequences %d\n", nFound);
                    // printResults(sequences, nSequences, nFound);
                }
                else
                {
                    printf("[+]Can't view results without uploading ref or seq\n");
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

        // Reset
        if (uploadedSeq)
        {
            free(sequence);
            uploadedSeq = 0;

            bzero(sequences, sizeof(sequences));
            nSequences = -1;
        }
        if (uploadedRef)
        {
            free(reference);
            uploadedRef = 0;
        }
        bzero(clientMsg, SIZE);
        close(clientfd);
        printf("[+]Connection terminated.\n");
    }
    return 0;
}