/*
 * newtest.c
 *
 * Copyright (c) 2014 Jeremy Garff <jer @ jers.net>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 *     1.  Redistributions of source code must retain the above copyright notice, this list of
 *         conditions and the following disclaimer.
 *     2.  Redistributions in binary form must reproduce the above copyright notice, this list
 *         of conditions and the following disclaimer in the documentation and/or other materials
 *         provided with the distribution.
 *     3.  Neither the name of the owner nor the names of its contributors may be used to endorse
 *         or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
* Name: main.c
* Description: This file contains the socket client code for the AESD final project titled :Image streaming and capturing. The LED matrix library
* 			   has been leveraged for this. The socket client and multithreaded implementation has been build upon this.
* Modified by: Gautama Gandhi
*/


static char VERSION[] = "XX.YY.ZZ";

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <errno.h>
#include <syslog.h>
#include <netdb.h>
#include <linux/fs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#include "clk.h"
#include "gpio.h"
#include "dma.h"
#include "pwm.h"
#include "version.h"

#include "ws2811.h"

#define ARRAY_SIZE(stuff)       (sizeof(stuff) / sizeof(stuff[0]))

// defaults for cmdline options
#define TARGET_FREQ             WS2811_TARGET_FREQ
#define GPIO_PIN                18
#define DMA                     10
//#define STRIP_TYPE            WS2811_STRIP_RGB		// WS2812/SK6812RGB integrated chip+leds
#define STRIP_TYPE              WS2811_STRIP_GRB		// WS2812/SK6812RGB integrated chip+leds
//#define STRIP_TYPE            SK6812_STRIP_RGBW		// SK6812RGBW (NOT SK6812RGB)

#define WIDTH                   16
#define HEIGHT                  16
#define LED_COUNT               (WIDTH * HEIGHT)

#define INPUT_BUFFER_SIZE 768
#define MGMT_BUFFER_SIZE 50

int width = WIDTH;
int height = HEIGHT;
int led_count = LED_COUNT;

int clear_on_exit = 0;

ws2811_t ledstring =
{
    .freq = TARGET_FREQ,
    .dmanum = DMA,
    .channel =
    {
        [0] =
        {
            .gpionum = GPIO_PIN,
            .invert = 0,
            .count = LED_COUNT,
            .strip_type = STRIP_TYPE,
            .brightness = 255,
        },
        [1] =
        {
            .gpionum = 0,
            .invert = 0,
            .count = 0,
            .brightness = 0,
        },
    },
};

ws2811_led_t *matrix;

static uint8_t running = 1;
static uint8_t running_mgmt = 1;

// Unsigned 32 bit led_matrix array to represent the LEDs
uint32_t led_matrix[LED_COUNT]; 

// Global variables for the Management Socket connected to the OpenWRT server
int mgmt_fd;
const char *openwrt_server_ip_addr = "192.168.1.1";
const char *openwrt_port_num = "9998";
struct addrinfo *mgmtservinfo; // Points to results

pthread_t client_thread_id; // Client Thread

int is_client_running = 0; // Variable to indicate if the client is running

// Render client socket information
int client_socketfd;
struct addrinfo *clientservinfo; // Points to results
const char *camera_server_ip_addr = "192.168.2.163";
const char *camera_server_port_num = "9000";

//Function to process the input array and assign values for the LED matrix
void process_array(unsigned char *input_array, int array_size)
{
	for (int i = 0; i < LED_COUNT; i++) {
		// Storing values received from the input array onto the led_matrix array
		// The input array format is R, G, B: this is packed into the led_matrix array
		led_matrix[i] = (((input_array[3*i] << 5) / 256) << 16) | (((input_array[3*i+1]<< 5) / 256) << 8) | (((input_array[3*i+2]<< 5) / 256));
	}
}

// Render the matrix by storing the values onto the ledstring structure
void matrix_render(void)
{
    int x, y;

    for (x = 0; x < width; x++)
    {
        for (y = 0; y < height; y++)
        {
            ledstring.channel[0].leds[(y * width) + x] = matrix[y * width + x];
        }
    }
}

// Clear the matrix by assigning zeros to each LED
void matrix_clear(void)
{
    int x, y;

    for (y = 0; y < (height ); y++)
    {
        for (x = 0; x < width; x++)
        {
            matrix[y * width + x] = 0;
        }
    }
}

// Function to display the color "Blue" on every LED
void test_matrix(void)
{
	for (int i = 0; i < 256; i++) {
		matrix[i] = 0x00000020;
	}
}

// Fill Matrix function for an 8x8 LED Matrix strip
void fill_matrix_8x8(void)
{
	for (int i = 0; i <= 7; i++) {
		matrix[i] = led_matrix[56+i];
	}
	for (int i = 8; i <= 15; i++) {
		matrix[i] = led_matrix[63-i];
	}
	for (int i = 16; i <= 23; i++) {
		matrix[i] = led_matrix[24+i];
	}
	for (int i = 24; i <= 31; i++) {
		matrix[i] = led_matrix[63-i];
	}
	for (int i = 32; i <= 39; i++) {
		matrix[i] = led_matrix[i-8];
	}
	for (int i = 40; i <= 47; i++) {
		matrix[i] = led_matrix[63-i];
	}
	for (int i = 48; i <= 55; i++) {
		matrix[i] = led_matrix[i-40];
	}
	for (int i = 56; i <= 63; i++) {
		matrix[i] = led_matrix[63-i];
	}

}

// Fill Matrix function for a 16x16 LED Matrix strip
void fill_matrix_16x16(void)
{
	// Values have been hardcoded based on the matrix layout
	for (int i = 0; i <= 15; i++) {
		matrix[i] = led_matrix[240+i];
	}
	for (int i = 16; i <= 31; i++) {
		matrix[i] = led_matrix[255-i];
	}
	for (int i = 32; i <= 47; i++) {
		matrix[i] = led_matrix[176+i];
	}
	for (int i = 48; i <= 63; i++) {
		matrix[i] = led_matrix[255-i];
	}
	for (int i = 64; i <= 79; i++) {
		matrix[i] = led_matrix[112+i];
	}
	for (int i = 80; i <= 95; i++) {
		matrix[i] = led_matrix[255-i];
	}
	for (int i = 96; i <= 111; i++) {
		matrix[i] = led_matrix[48+i];
	}
	for (int i = 112; i <= 127; i++) {
		matrix[i] = led_matrix[255-i];
	}
	for (int i = 128; i <= 143; i++) {
		matrix[i] = led_matrix[i-16];
	}
	for (int i = 144; i <= 159; i++) {
		matrix[i] = led_matrix[255-i];
	}
	for (int i = 160; i <= 175; i++) {
		matrix[i] = led_matrix[i-80];
	}
	for (int i = 176; i <= 191; i++) {
		matrix[i] = led_matrix[255-i];
	}
	for (int i = 192; i <= 207; i++) {
		matrix[i] = led_matrix[i-144];
	}
	for (int i = 208; i <= 223; i++) {
		matrix[i] = led_matrix[255-i];
	}
	for (int i = 224; i <= 239; i++) {
		matrix[i] = led_matrix[i-208];
	}
	for (int i = 240; i <= 255; i++) {
		matrix[i] = led_matrix[255-i];
	}

}

static void sig_handler(int signum)
{
	(void)(signum);
	if (signum == SIGINT)
        printf("Caught signal SIGINT, exiting\n");
    else if (signum == SIGTERM)
        printf("Caught signal SIGTERM, exiting\n");
    running = 0;
	running_mgmt = 0;
	clear_on_exit = 1;
}

static void setup_handlers(void)
{
    struct sigaction sa =
    {
        .sa_handler = sig_handler,
    };

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

/************************************************
	MULTITHREAD LOGIC START
*************************************************/

void *render_thread(void *arg)
{
	ws2811_return_t ret;

	unsigned char render_buffer[INPUT_BUFFER_SIZE];

    int connect_status = connect(client_socketfd, clientservinfo->ai_addr, clientservinfo->ai_addrlen);

	freeaddrinfo(clientservinfo);

	if (connect_status == -1) {
		printf("Error in connecting to the camera server!\n");
	}

	if (connect_status == 0) {
		printf("Connection to camera server successful.\n");
	}

    while (running)
    {
		// bytes_received stores the number of bytes received using the recv system call from the socket
        int bytes_received = recv(client_socketfd, render_buffer, sizeof(render_buffer), 0);

		// Only process the array and render the image if bytes received is equal to 768 bytes
		if (bytes_received == INPUT_BUFFER_SIZE) {

			process_array(render_buffer, INPUT_BUFFER_SIZE);
			fill_matrix_16x16();
			matrix_render();

			if ((ret = ws2811_render(&ledstring)) != WS2811_SUCCESS)
			{
				fprintf(stderr, "ws2811_render failed: %s\n", ws2811_get_return_t_str(ret));
				break;
			}

			// Managing Frame Rate using a delay
			usleep(1000000 / 120);
		}
    }
}

// Setting management socket that connects to OpenWRT server
int setup_management_socket()
{
	int status = 0;
    struct addrinfo hints;
    // struct addrinfo *servinfo; // Points to results

    memset(&hints, 0, sizeof(hints)); // Empty struct
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

	// Connecting to OpenWRT server
    if ((status = getaddrinfo(openwrt_server_ip_addr, openwrt_port_num, &hints, &mgmtservinfo)) != 0) {
        printf("Error in getting addrinfo! Error number is %d\n", errno);
		return status;
    }

    mgmt_fd = socket(mgmtservinfo->ai_family, mgmtservinfo->ai_socktype, mgmtservinfo->ai_protocol);

    if (mgmt_fd == -1)
    {
        printf("Error creating socket! Error number is %d\n", errno);
		status = -1;
    }
	return status;
}

// Setup client render socket
int setup_client_socket()
{
	int status = 0;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints)); // Empty struct
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

	// Connecting to Camera server
	// 
    if ((status = getaddrinfo(camera_server_ip_addr, camera_server_port_num, &hints, &clientservinfo)) != 0) {
        printf("Error in getting addrinfo! Error number is %d\n", errno);
        return status;
    }

    client_socketfd = socket(clientservinfo->ai_family, clientservinfo->ai_socktype, clientservinfo->ai_protocol);

    if (client_socketfd == -1)
    {
        printf("Error creating socket! Error number is %d\n", errno);
		status = -1;
    }

	status = pthread_create(&client_thread_id, NULL, render_thread, (void *)0);
	if (status == -1) {
		printf("Unable to create thread\n");
	}

	is_client_running = 1; // Updating the client running flag when the socket is created 
	return status;
}

// Function to cleanup the socket client connected to the camera server
int close_client_socket()
{
	int status = 0;
	status = close(client_socketfd);
	// Exit in case of failure
	if (status == -1) {
		printf("Error closing socket\n");
		goto ret;
	}
	status = pthread_cancel(client_thread_id);
	if (status != 0) {
		printf("Error cancelling thread\n");
		goto ret;
	}
	status = pthread_join(client_thread_id, NULL);
	if (status != 0) {
		printf("Error joining thread\n");
		goto ret;
	}
	is_client_running = 0;
	ret: return status;
}

int main(int argc, char *argv[])
{

	printf("Socket Client\n");

	setup_handlers();

	ws2811_return_t ret;

    matrix = malloc(sizeof(ws2811_led_t) * width * height);

    if ((ret = ws2811_init(&ledstring)) != WS2811_SUCCESS) {
        fprintf(stderr, "ws2811_init failed: %s\n", ws2811_get_return_t_str(ret));
        return ret;
    }

	int return_status = setup_management_socket();
	if (return_status == -1) {
		printf("Error setting up management socket; exiting\n");
		goto cleanup;
	}

	int connect_status = connect(mgmt_fd, mgmtservinfo->ai_addr, mgmtservinfo->ai_addrlen);

	if (connect_status == -1) {
		printf("Failed to connect to the OpenWRT server\n");
		goto close_socket;
	} 

	unsigned char buffer[MGMT_BUFFER_SIZE]; // Managgement buffer

	while (running_mgmt) {
		// bytes_received stores the number of bytes received using the recv system call from the mgmt socket
        int bytes_received = recv(mgmt_fd, buffer, sizeof(buffer), 0);

		// Only process the array and render the image if bytes received is equal to 768 bytes
		if (bytes_received > 0) {
			if (!strcasecmp(buffer, "start")) {
				if (!is_client_running) {
					int status = setup_client_socket();
					if (status == -1) {
						printf("Unable to setup client socket\n");
						goto close_socket;
					}
				}	
			} 
			else if (!strcasecmp(buffer, "stop")) {
				if (is_client_running) {
					int status = close_client_socket();
					if (status == -1) {
						printf("Unable to close client socket\n");
						goto close_socket;
					}
				}	
			}
		}
	}

	if (clear_on_exit) {
		matrix_clear();
		matrix_render();
		ws2811_render(&ledstring);
    }

	close_socket: close(mgmt_fd);
    cleanup: ws2811_fini(&ledstring);

    return 0;
}
