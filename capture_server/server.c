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

#include "seq.h"

#define BACKLOG 10
#define INITIAL_BUFFER_SIZE 768

void initialize_camera();
void shutdown_camera();

// int main(int argc, char **argv)
// {
//     unsigned char *image_buffer;

//     initialize_camera();

//     image_buffer = get_processed_image_data();
//     image_buffer = get_processed_image_data();
//     image_buffer = get_processed_image_data();
//     image_buffer = get_processed_image_data();

//     for (int k = 0; k < 192; k++)
//     {
//         printf("%0d,", image_buffer[k]);

//         if (k % 15 == 0)
//             printf("\n");
//     }
//     printf("\n");

//     shutdown_camera();
//     return 0;
// }

void initialize_camera()
{
    open_device();
    init_device();
    start_capturing();
}

void shutdown_camera()
{
    stop_capturing();
    uninit_device();
    close_device();
}

// Global Variables
int socketfd;         // Socket File Descriptor
int new_fd;           // File Descriptor used to receive and send data
char *storage_buffer; // Pointer to storage buffer

// Signal handler for SIGINT and SIGTERM signals
void sig_handler(int signum)
{
    if (signum == SIGINT)
        syslog(LOG_DEBUG, "Caught signal SIGINT, exiting");
    else if (signum == SIGTERM)
        syslog(LOG_DEBUG, "Caught signal SIGTERM, exiting");

    // Freeing storage buffer allocated memory
    free(storage_buffer);

    // Closing socketfd FD
    int status = close(socketfd);
    if (status == -1)
    {
        syslog(LOG_ERR, "Unable to close socket FD with error %d", errno);
    }

    // Closing testfile FD
    status = close(new_fd);
    if (status == -1)
    {
        syslog(LOG_ERR, "Unable to close new_fd FD with error %d", errno);
    }
    closelog();
    exit(EXIT_SUCCESS);
}

// Main function
int main(int argc, char **argv)
{
    unsigned char *image_buffer;
    initialize_camera();
    // Opening Log to use syslog
    openlog(NULL, 0, LOG_USER);

    // Flag that is set when "-d"
    int daemon_flag = 0;

    if (argc == 2)
    {
        char *daemon_argument = argv[1];
        if (strcmp(daemon_argument, "-d") != 0)
        {
            printf("Invalid argument to run as daemon! Expected \"-d\"\n");
        }
        else
        {
            daemon_flag = 1;
        }
    }

    int yes = 1; // USed in the setsockopt function

    printf("Socket Programming 101!\n");

    signal(SIGINT, sig_handler);  // Register signal handler for SIGINT
    signal(SIGTERM, sig_handler); // Register signal handler for SIGTERM

    // Socket SETUP begin
    // Getting a socketfd
    socketfd = socket(AF_INET, SOCK_STREAM, 0);

    if (socketfd == -1)
    {
        printf("Error creating socket! Error number is %d\n", errno);
        return -1;
    }

    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo; // Points to results

    memset(&hints, 0, sizeof(hints)); // Empty struct
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, "9000", &hints, &servinfo)) != 0)
    {
        printf("Error in getting addrinfo! Error number is %d\n", errno);
        return -1;
    }

    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1)
    {
        printf("Error in setting socket FD's options! Error number is %d\n", errno);
        return -1;
    }

    status = bind(socketfd, servinfo->ai_addr, sizeof(struct addrinfo));
    if (status == -1)
    {
        printf("Error binding socket! Error number is %d\n", errno);
        return -1;
    }

    freeaddrinfo(servinfo);
    // Socket SETUP end

    status = listen(socketfd, BACKLOG);
    if (status == -1)
    {
        printf("Error listening to connections! Error number is %d\n", errno);
        return -1;
    }

    struct sockaddr_storage test_addr;

    socklen_t addr_size = sizeof test_addr;

    // Dynamically allocated storage buffer to store values received from the socket
    storage_buffer = (char *)malloc(INITIAL_BUFFER_SIZE);

    // Variable to store size of storage buffer
    int storage_buffer_size = INITIAL_BUFFER_SIZE;

    // Variable to store return value from recv
    int bytes_sent;

    // Variable to track the size of the storage buffer 1 indicates size is 1*1024; used when computing bytes to be written and memcpy operation on the storage buffer
    int storage_buffer_size_count = 1;

    // String to hold the IP address of the client; used when printing logs
    char address_string[INET_ADDRSTRLEN];

    while (1)
    {
        new_fd = accept(socketfd, (struct sockaddr *)&test_addr, &addr_size);

        if (new_fd == -1)
        {
            perror("accept");
        }

        struct sockaddr_in *p = (struct sockaddr_in *)&test_addr;
        printf("Accepted connection from %s\n", inet_ntop(AF_INET, &p->sin_addr, address_string, sizeof(address_string)));

        image_buffer = get_processed_image_data();
        
        bytes_sent = send(new_fd, image_buffer, sizeof(image_buffer), 0);
        printf("Bytes sent = %d\n", bytes_sent);

        syslog(LOG_DEBUG, "Closed connection from %s", address_string);
        close(new_fd);
    }

    
    return 0;
}