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
#include <unistd.h>
#include "seq.h"

#define BACKLOG (1)
#define FRAME_DELAY_MS (0)
#define INITIAL_BUFFER_SIZE (768)

void initialize_camera(void);
void shutdown_camera(void);
void setup_socket(void);
void sig_handler(int signum);

// Global Variables
int socketfd;         // Socket File Descriptor
int new_fd;           // File Descriptor used to receive and send data
char *storage_buffer; // Pointer to storage buffer

//-------------------------------------main--------------------------------------------------------------------------
int main(int argc, char **argv)
{
    unsigned char *image_buffer;

    printf("Initializing Camera and Allocating Buffers... \n");
    initialize_camera();
    // Opening Log to use syslog
    openlog(NULL, 0, LOG_USER);

    int yes = 1;
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

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
        printf("Error in getaddrinfo -> %d\n", errno);
        return -1;
    }

    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1)
    {
        printf("Error in sersockopt -> %d\n", errno);
        return -1;
    }

    status = bind(socketfd, servinfo->ai_addr, sizeof(struct addrinfo));
    if (status == -1)
    {
        printf("Error bindi -> %d\n", errno);
        return -1;
    }

    freeaddrinfo(servinfo);
    status = listen(socketfd, BACKLOG);
    if (status == -1)
    {
        printf("Error listening to connections! Error number is %d\n", errno);
        return -1;
    }

    struct sockaddr_storage test_addr;

    socklen_t addr_size = sizeof test_addr;
    storage_buffer = (char *)malloc(INITIAL_BUFFER_SIZE);
    int storage_buffer_size = INITIAL_BUFFER_SIZE;
    int bytes_sent;

    // String to hold the IP address of the client; used when printing logs
    char address_string[INET_ADDRSTRLEN];

accept_new:
    new_fd = accept(socketfd, (struct sockaddr *)&test_addr, &addr_size);

    if (new_fd == -1)
    {
        perror("accept");
    }

    struct sockaddr_in *p = (struct sockaddr_in *)&test_addr;
    printf("Accepted connection from %s\n", inet_ntop(AF_INET, &p->sin_addr, address_string, sizeof(address_string)));

    while (1)
    {

        image_buffer = get_processed_image_data();

        bytes_sent = send(new_fd, image_buffer, 768, 0);

        // printf("Bytes Sent -> %d \n", bytes_sent);

        if (bytes_sent == -1)
        {
            close(new_fd);
            goto accept_new;
        }

        if (FRAME_DELAY_MS)
            usleep(FRAME_DELAY_MS * 1000);
    }

    syslog(LOG_DEBUG, "Closed connection from %s", address_string);

    return 0;
}
//-------------------------------------sig_handler--------------------------------------------------------------------------
void sig_handler(int signum)
{
    if (signum == SIGINT)
        printf("Caught signal SIGINT, exiting");
    else if (signum == SIGTERM)
        printf("Caught signal SIGTERM, exiting");

    // Freeing storage buffer allocated memory
    free(storage_buffer);

    // Closing socketfd FD
    int status = close(socketfd);
    if (status == -1)
    {
        syslog(LOG_ERR, "Unable to close socket FD with error %d", errno);
    }

    status = close(new_fd);
    if (status == -1)
    {
        syslog(LOG_ERR, "Unable to close new_fd FD with error %d", errno);
    }
    closelog();
    exit(EXIT_SUCCESS);
}

//-------------------------------------initialize_camera--------------------------------------------------------------------------
void initialize_camera()
{
    open_device();
    init_device();
    start_capturing();
}

//-------------------------------------shutdown_camera--------------------------------------------------------------------------
void shutdown_camera()
{
    stop_capturing();
    uninit_device();
    close_device();
}
//-------------------------------------End--------------------------------------------------------------------------
