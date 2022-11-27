/**
* File Name: aesdsocket.c
* File Desciption: This file contains the main function for AESD Assignment 5 Part 1.
* File Author: Gautama Gandhi
* Reference: Linux man pages and Linux System Programming Textbook
**/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <syslog.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <linux/fs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#define BACKLOG 10 // Setting 10 as the connection request limit while listening
#define INITIAL_BUFFER_SIZE 1024 // Buffer size for receive and storage bufferss

// Global Variables
int socketfd; // Socket File Descriptor
int testfile_fd; // Testfile File Descriptor
int new_fd; // File Descriptor used to receive and send data
char *storage_buffer; // Pointer to storage buffer


// Signal handler for SIGINT and SIGTERM signals
void sig_handler(int signum)
{
    if (signum == SIGINT)
        syslog(LOG_DEBUG, "Caught signal SIGINT, exiting");
    else if (signum == SIGTERM)
        syslog(LOG_DEBUG, "Caught signal SIGTERM, exiting");

    exit(EXIT_SUCCESS);
}

// Main function
int main(int argc, char **argv)
{

    printf("Socket Client\n");

    signal(SIGINT, sig_handler); // Register signal handler for SIGINT
    signal(SIGTERM, sig_handler); // Register signal handler for SIGTERM

    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo; // Points to results

    memset(&hints, 0, sizeof(hints)); // Empty struct
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;


    if ((status = getaddrinfo("127.0.0.1", "9000", &hints, &servinfo)) != 0) {
        printf("Error in getting addrinfo! Error number is %d\n", errno);
        return -1;
    }

    // Socket SETUP begin
    // Getting a socketfd
    socketfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    if (socketfd == -1) {
        printf("Error creating socket! Error number is %d\n", errno);
        return -1;
    }

    int client_fd = connect(socketfd, servinfo->ai_addr, servinfo->ai_addrlen);
 
    // Send data to socket
    char *message = "Pls Work!\n";

    int bytes_sent = send(socketfd, message, strlen(message), 0);
    if (bytes_sent == strlen(message))
        printf("Success\n");

    char buffer[20];
    int bytes_received = recv(socketfd, buffer, sizeof(buffer), 0);

    for (int i = 0; i < 10; i++) {
        printf("%c", buffer[i]);
    }

    printf("\n");


    freeaddrinfo(servinfo);
    // Socket SETUP end


    return 0;
}