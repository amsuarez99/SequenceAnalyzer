# Sequence Analyzer

## About The Project
This program analyzes if a given sequence is present in a DNA genome.

## Instructions
This program can be run as a normal program or as a daemon.

### Prerequisites
To run the program you must be inside the root of the project directory

### Run the server normally
```sh
$ gcc -o bin/server server.c
$ bin/server
```

### Run the server as a daemon
```sh
$ gcc -o bin/serverd serverd.c
$ bin/serverd
```

### Connect to the server
```sh
$ gcc -o bin/client client.c
$ bin/client
```