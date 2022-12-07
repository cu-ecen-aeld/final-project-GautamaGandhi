
/***************************************************************************
 * AESD Final Project
 * Author:  Chinmay Shalawadi
 * Institution: University of Colorado Boulder
 * Mail id: chsh1552@colorado.edu
 * References: AESDSocket,Wikipedia, ChatGPT & stb header library
 ***************************************************************************/

// #define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sched.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <getopt.h>
#include <fcntl.h>
#include <signal.h>
#include <linux/fs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
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
void setup_mansocket();
void set_scheduler(int cpu_id, int prio_offset);
void *cam_server(void *threadp);
void setup_camsocket();

// Global Variables
int data_socketfd; 
int data_newfd;    

int man_socketfd; 
int man_newfd;    
int camera_on = 0;

struct addrinfo *man_servinfo;
pthread_t camthread;

#define SCHED_POLICY SCHED_FIFO


//-------------------------------------setup_mansocket--------------------------------------------------------------------------
void setup_mansocket()
{

    int status;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo("192.168.1.1", "9999", &hints, &man_servinfo)) != 0)
    {
        printf("Error in getting addrinfo! Error number is %d\n", errno);
        // return -1;
    }

    man_socketfd = socket(man_servinfo->ai_family, man_servinfo->ai_socktype, man_servinfo->ai_protocol);

    if (man_socketfd == -1)
    {
        printf("Error creating socket! Error number is %d\n", errno);
        // return -1;
    }
}

//-------------------------------------setup_camsocket--------------------------------------------------------------------------
void setup_camsocket()
{
    int yes = 1;
    data_socketfd = socket(AF_INET, SOCK_STREAM, 0);

    if (data_socketfd == -1)
    {
        printf("Error creating socket! Error number is %d\n", errno);
        // return -1;
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
        // return -1;
    }

    if (setsockopt(data_socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1)
    {
        printf("Error in sersockopt -> %d\n", errno);
        // return -1;
    }

    status = bind(data_socketfd, servinfo->ai_addr, sizeof(struct addrinfo));
    if (status == -1)
    {
        printf("Error bindi -> %d\n", errno);
        // return -1;
    }

    freeaddrinfo(servinfo);
    status = listen(data_socketfd, BACKLOG);
    if (status == -1)
    {
        printf("Error listening to connections! Error number is %d\n", errno);
        // return -1;
    }
}

//-------------------------------------*cam_server--------------------------------------------------------------------------
void *cam_server(void *threadp)
{
    struct sockaddr_storage test_addr;

    socklen_t addr_size = sizeof test_addr;
    int bytes_sent;

    char address_string[INET_ADDRSTRLEN];
    unsigned char *image_buffer;

    printf("Server Started, waiting for connections \n");

    initialize_camera();

accept:
    data_newfd = accept(data_socketfd, (struct sockaddr *)&test_addr, &addr_size);

    if (data_newfd == -1)
    {
        perror("accept");
    }

    struct sockaddr_in *p = (struct sockaddr_in *)&test_addr;
    printf("Accepted connection from %s\n", inet_ntop(AF_INET, &p->sin_addr, address_string, sizeof(address_string)));

    while (1)
    {

        if (camera_on)
            image_buffer = get_processed_image_data();

        bytes_sent = send(data_newfd, image_buffer, 768, 0);

        // printf("Bytes Sent -> %d \n", bytes_sent);

        if (bytes_sent == -1)
        {
            printf("going to accept new \n");
            goto accept;
        }

        if (FRAME_DELAY_MS)
            usleep(FRAME_DELAY_MS * 1000);
    }

exit:
    printf("Closed connection from %s", address_string);

    pthread_exit((void *)0);
}

//-------------------------------------main--------------------------------------------------------------------------
int main(int argc, char **argv)
{
    printf("Initializing Camera and Allocating Buffers... \n");

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGPIPE, SIG_IGN);

    setup_mansocket();
    setup_camsocket();

    char buffer[50];
    man_newfd = connect(man_socketfd, man_servinfo->ai_addr, man_servinfo->ai_addrlen);

    while (1)
    {

        int bytes_received = recv(man_socketfd, buffer, 768, 0);

        if (bytes_received == 0)
        {
            printf("bytes0\n");
        }

        if (bytes_received > 0)
        {
            if (!strcasecmp(&buffer[0], "start"))
            {
                pthread_create(&camthread, NULL, cam_server, (void *)0);
            }

            if (!strcasecmp(&buffer[0], "stop"))
            {
                printf("Stopping the camera server \n");
                pthread_cancel(camthread);
                pthread_join(camthread, NULL);
                shutdown_camera();
                // close(data_);
            }
        }
    }

    freeaddrinfo(man_servinfo);

    return 0;
}

//-------------------------------------sig_handler--------------------------------------------------------------------------
void sig_handler(int signum)
{
    if (signum == SIGINT)
        printf("Caught signal SIGINT, exiting");
    else if (signum == SIGTERM)
        printf("Caught signal SIGTERM, exiting");

    // shutdown_camera();

    // Closing data_socketfd FD
    int status = close(data_socketfd);
    if (status == -1)
    {
        printf("Unable to close socket FD with error %d", errno);
    }

    status = close(data_newfd);
    if (status == -1)
    {
        printf("Unable to close data_newfd FD with error %d", errno);
    }
    exit(EXIT_SUCCESS);
}

//-------------------------------------initialize_camera--------------------------------------------------------------------------
void initialize_camera()
{
    if (!camera_on)
    {
        printf("Camera turned on \n");
        open_device();
        init_device();
        start_capturing();
        camera_on = 1;
    }
}

//-------------------------------------shutdown_camera--------------------------------------------------------------------------
void shutdown_camera()
{
    if (camera_on)
    {
        printf("Camera turned off \n");
        stop_capturing();
        uninit_device();
        close_device();
        camera_on = 0;
    }
}
//-------------------------------------End--------------------------------------------------------------------------
