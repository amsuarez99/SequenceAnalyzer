#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <syslog.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#define SIZE 1024
#define DEBUG 1
#define PORT 8000
#define NTHREADS 10

char cwd[FILENAME_MAX];
pthread_mutex_t lock;

struct thread_attributes
{
    size_t thread_num;
    size_t nThreads;
    size_t nSequences;
    int *nFound;
    char *ref;
    char *refcpy;
    struct _Sequence *sequences;
};

struct _Sequence
{
    char *sequenceString;
    size_t size;
    int position;
};

static void signalHandler(int signo) {
    if(signo == SIGTERM) {
        syslog(LOG_NOTICE, "[+]Terminated Daemon");
        closelog();
        exit(EXIT_SUCCESS);
    }
}

static void daemonize()
{
    pid_t pid;

    /* Fork off the parent process */
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* On success: The child process becomes session leader */
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    /* Catch, ignore and handle signals */
    signal(SIGTERM, signalHandler);

    /* Fork off for the second time*/
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* Set new file permissions */
    umask(0);

    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    chdir("/Users/amsua/Documents/Tec/Feb-2021/progra/adn");

    /* Close all open file descriptors */
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x >= 0; x--)
    {
        close(x);
    }

    /* Open the log file */
    openlog("ANALYZER_SERVERD", LOG_PID, LOG_DAEMON);
}


float getPercentage(char *refcpy)
{

    int i = 0;
    char c;
    int ocurrences = 0;
    while ((c = refcpy[i++]) != '\0')
        if (c == 'x')
            ocurrences++;
    return (float)ocurrences / (float)strlen(refcpy) * 100;
}

void *findSeq(void *x)
{
    struct thread_attributes arguments;
    arguments = *((struct thread_attributes *)x);
    char *offset;
    int index;
    int load = arguments.nSequences / arguments.nThreads;
    // Add remaining load to the last thread
    if (arguments.thread_num == (arguments.nThreads - 1))
    {
        load += arguments.nSequences - (load * arguments.nThreads);
    }
    for (int j = 0; j < load; j++)
    {
        index = (arguments.thread_num * arguments.nSequences / arguments.nThreads) + j;
        if ((offset = strstr(arguments.ref, arguments.sequences[index].sequenceString)) != NULL)
        {
            arguments.sequences[index].position = offset - arguments.ref;
            pthread_mutex_lock(&lock);
            *arguments.nFound = *arguments.nFound + 1;
            // strncpy(arguments.refcpy + arguments.sequences[index].position, "x", arguments.sequences[index].size);
            memset(arguments.refcpy + arguments.sequences[index].position, 'x',
                   arguments.sequences[index].size);
            pthread_mutex_unlock(&lock);
        }
    }
    return NULL;
}

void sendRes(int socketfd, struct _Sequence *seqs, size_t nSeqs, size_t nFound, float percentage)
{
    char buffer[SIZE] = {0};
    int j;
    sprintf(buffer, "=== RESULTS ===\n");
    send(socketfd, buffer, SIZE, 0);
    bzero(buffer, SIZE);
    for (size_t i = 0; i < nSeqs; i++)
    {
        if (seqs[i].position != -1)
            sprintf(buffer, "Sequence %zu: %*.*s (HEAD) found at character %d\n",
                    i,
                    1, 10,
                    seqs[i].sequenceString,
                    seqs[i].position);
        else
            sprintf(buffer, "Sequence %zu: %*.*s (HEAD) not found\n",
                    i,
                    1, 10,
                    seqs[i].sequenceString);
        send(socketfd, buffer, SIZE, 0);
        bzero(buffer, SIZE);
    }

    j = sprintf(buffer, "Covered %f%% of reference genome\n", percentage);
    j += sprintf(buffer + j, "Mapped %zu sequences\n", nFound);
    j += sprintf(buffer + j, "Couldn\'t map %zu sequences\n", nSeqs - nFound);
    send(socketfd, buffer, SIZE, 0);
    bzero(buffer, SIZE);
    sprintf(buffer, "DONE");
    send(socketfd, buffer, SIZE, 0);
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
    printf("[+]Starting daemon...\n");
    daemonize();
    char *ip = "127.0.0.1";
    int serverfd, clientfd, readLength;
    socklen_t addr_size;
    struct sockaddr_in server, client;

    char clientMsg[SIZE] = {0};
    char *buffer = NULL, *sequence = NULL, *reference = NULL, *refCpy = NULL;
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
    syslog(LOG_NOTICE, "[+]Socket created successfully.\n");

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
    syslog(LOG_NOTICE, "[+]Binding successfull.\n");

    while (1)
    {
        // Listen
        if (listen(serverfd, 1) != 0)
        {
            perror("[-]Error in listen");
            exit(EXIT_FAILURE);
        }
        syslog(LOG_NOTICE, "[+]Listening....\n");

        addr_size = sizeof(struct sockaddr_in);
        if ((clientfd = accept(serverfd, (struct sockaddr *)&client, (socklen_t *)&addr_size)) < 0)
        {
            perror("[-]Error in accept");
            exit(EXIT_FAILURE);
        }
        syslog(LOG_NOTICE, "[+]Connection accepted.\n");

        // Receives length of next message
        while ((readLength = recv(clientfd, clientMsg, sizeof(clientMsg), 0)))
        {
            int nextMsgLength = atoi(clientMsg);
            syslog(LOG_NOTICE, "[+]Message length: %d\n", nextMsgLength);

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
                syslog(LOG_NOTICE, "[+]Received sequence: %s (SHOWING TAIL)\n", sequence + strlen(sequence) - 10);

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
                syslog(LOG_NOTICE, "[+]Received ref: %s (SHOWING TAIL)\n", reference + strlen(reference) - 10);
            }
            else if (option == 'R')
            {
                if (uploadedSeq && uploadedRef)
                {
                    refCpy = strdup(reference);
                    nFound = 0;
                    syslog(LOG_NOTICE, "[+]Client wants results...\n");
                    // strcpy(clientMsg, "====RESULTS====\nPercentage covered: 20%\nSequences used:\nAGAGAGGATT\nAGGGAGGAGT\n");
                    const size_t nThreads = nSequences < NTHREADS ? nSequences : NTHREADS; // Get min
                    pthread_t threads[nThreads];
                    struct thread_attributes thread_args[nThreads];

                    if (pthread_mutex_init(&lock, NULL) != 0)
                    {
                        perror("Mutex init failed\n");
                        exit(EXIT_FAILURE);
                    }

                    // Spawn a thread for each row
                    size_t load = nSequences / nThreads;
                    for (size_t i = 0; i < nThreads; ++i)
                    {
                        struct thread_attributes a = {i, nThreads, nSequences, &nFound, reference, refCpy, sequences};
                        thread_args[i] = a;
                        pthread_create(&threads[i], NULL, findSeq, (void *)&thread_args[i]);
                    }

                    // Wait for threads to finish
                    for (size_t i = 0; i < nThreads; ++i)
                    {
                        pthread_join(threads[i], NULL);
                    }
                    pthread_mutex_destroy(&lock);

                    syslog(LOG_NOTICE, "[+]Found %d sequences\n", nFound);
                    double percentage = getPercentage(refCpy);
                    sendRes(clientfd, sequences, nSequences, nFound, percentage);
                    free(refCpy);
                }
                else
                {
                    syslog(LOG_NOTICE, "[-]Can't view results without uploading ref or seq\n");
                    strcpy(clientMsg, "Can't view results without uploading ref or seq\n");
                    send(clientfd, clientMsg, SIZE, 0);
                }
            }

            // SEND OPERATION COMPLETE
            send(clientfd, "OK", 3, 0);
            bzero(clientMsg, SIZE);
            free(buffer);
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
        syslog(LOG_NOTICE, "[+]Connection terminated.\n");
    }
    return 0;
}