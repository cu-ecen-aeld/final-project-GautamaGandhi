
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
void sig_handler(int signum);
int setup_mansocket();
void set_scheduler(int cpu_id, int prio_offset);
void *cam_server(void *threadp);
int setup_camsocket();
int get_luma();

// Global Variables

// data socket variables
int data_socketfd;
int data_newfd;

// management socket variables
int man_socketfd;
int man_newfd;

// flags
int camera_on = 0;
int luma = 0;

struct addrinfo *man_servinfo;
pthread_t camthread;

//-------------------------------------setup_mansocket--------------------------------------------------------------------------
int setup_mansocket()
{
    int status, retval = 0;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo("192.168.1.1", "9999", &hints, &man_servinfo)) != 0)
    {
        printf("Error in getting addrinfo! Error number is %d\n", errno);
        retval = -1;
    }

    man_socketfd = socket(man_servinfo->ai_family, man_servinfo->ai_socktype, man_servinfo->ai_protocol);

    if (man_socketfd == -1)
    {
        printf("Error creating socket! Error number is %d\n", errno);
        retval = -1;
    }

    return retval;
}

//-------------------------------------setup_camsocket--------------------------------------------------------------------------
int setup_camsocket()
{
    int yes = 1, retval = 0;
    data_socketfd = socket(AF_INET, SOCK_STREAM, 0);

    if (data_socketfd == -1)
    {
        printf("Error creating socket! Error number is %d\n", errno);
        retval = -1;
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
        retval = -1;
    }

    if (setsockopt(data_socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1)
    {
        printf("Error in sersockopt -> %d\n", errno);
        retval = -1;
    }

    status = bind(data_socketfd, servinfo->ai_addr, sizeof(struct addrinfo));
    if (status == -1)
    {
        printf("Error bindi -> %d\n", errno);
        retval = -1;
    }

    freeaddrinfo(servinfo);
    status = listen(data_socketfd, BACKLOG);
    if (status == -1)
    {
        printf("Error listening to connections! Error number is %d\n", errno);
        retval = -1;
    }

    return retval;
}

//-------------------------------------*cam_server--------------------------------------------------------------------------
void *cam_server(void *threadp)
{
    int bytes_sent;
    struct sockaddr_storage test_addr;
    socklen_t addr_size = sizeof test_addr;

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
    printf("test\n");

    // Registering Singnal handlers
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    // To avoid being terminated by SIG_PIPE, ignore it
    signal(SIGPIPE, SIG_IGN);

    if (setup_mansocket() < 0)
        printf("ERR in setting up Management Socket\n");

    if (setup_camsocket() < 0)
        printf("ERR in setting up Camera socket\n");

    char buffer[50];

reconnect_man:
    man_newfd = connect(man_socketfd, man_servinfo->ai_addr, man_servinfo->ai_addrlen);

    while (1)
    {
        int bytes_received = recv(man_socketfd, buffer, 50, 0);

        if (bytes_received == 0)
        {
            goto reconnect_man;
        }

        if (bytes_received > 0)
        {
            if (!strcasecmp(&buffer[0], "start"))
            {
                if (!camera_on)
                    pthread_create(&camthread, NULL, cam_server, (void *)0);
            }

            if (!strcasecmp(&buffer[0], "stop"))
            {
                printf("Stopping the camera server \n");
                pthread_cancel(camthread);
                pthread_join(camthread, NULL);
                shutdown_camera();
            }

            if (!strcasecmp(&buffer[0], "G"))
            {
                if (luma)
                    luma = 0;
                else
                    luma = 1;
            }
        }
    }
    
    return 0;
}
//-------------------------------------get_luma--------------------------------------------------------------------------
int get_luma(){
    return luma;
}
//-------------------------------------sig_handler--------------------------------------------------------------------------
void sig_handler(int signum)
{
    if (signum == SIGINT)
        printf("Caught signal SIGINT, exiting");
    else if (signum == SIGTERM)
        printf("Caught signal SIGTERM, exiting");

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
    
    freeaddrinfo(man_servinfo);
    close(man_socketfd);
    
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
