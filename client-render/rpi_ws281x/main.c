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
* main.c: Socket Client code for LED matrix render
* Reference: https://github.com/jgarff/rpi_ws281x
* Modified by: Gautama Gandhi
*
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
#include <getopt.h>

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
#include <time.h>


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

// Unsigned 32 bit led_matrix array to represent the LEDs
uint32_t led_matrix[LED_COUNT]; 

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
        printf("Caught signal SIGINT, exiting");
    else if (signum == SIGTERM)
        printf("Caught signal SIGTERM, exiting");
    running = 0;
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

int main(int argc, char *argv[])
{

	printf("Socket Client\n");

	unsigned char *image_buffer;

    ws2811_return_t ret;

    sprintf(VERSION, "%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);

    matrix = malloc(sizeof(ws2811_led_t) * width * height);

    setup_handlers();

    if ((ret = ws2811_init(&ledstring)) != WS2811_SUCCESS)
    {
        fprintf(stderr, "ws2811_init failed: %s\n", ws2811_get_return_t_str(ret));
        return ret;
    }

    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo; // Points to results

    memset(&hints, 0, sizeof(hints)); // Empty struct
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo("127.0.0.1", "9000", &hints, &servinfo)) != 0)
    {
        printf("Error in getting addrinfo! Error number is %d\n", errno);
        return -1;
    }

    // Socket SETUP begin
    // Getting a socketfd
    int socketfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    if (socketfd == -1)
    {
        printf("Error creating socket! Error number is %d\n", errno);
        return -1;
    }

    unsigned char buffer[INPUT_BUFFER_SIZE];

    int connect_status = connect(socketfd, servinfo->ai_addr, servinfo->ai_addrlen);

	freeaddrinfo(servinfo);

	if (connect_status == -1) {
		printf("Error in connection!\n");
	}

	if (connect_status == 0) {
		printf("Connection to server successfull.\n");
	}

    while (running)
    {
		// bytes_received stores the number of bytes received using the recv system call from the socket
        int bytes_received = recv(socketfd, buffer, sizeof(buffer), 0);

		// Only process the array and render the image if bytes received is equal to 768 bytes
		if (bytes_received == INPUT_BUFFER_SIZE) {

			process_array(buffer, INPUT_BUFFER_SIZE);
			fill_matrix_16x16();
			matrix_render();

			if ((ret = ws2811_render(&ledstring)) != WS2811_SUCCESS)
			{
				fprintf(stderr, "ws2811_render failed: %s\n", ws2811_get_return_t_str(ret));
				break;
			}

			// 15 frames /sec
			usleep(1000000 / 120);
		}
    }

	close(socketfd);

    if (clear_on_exit) {
		matrix_clear();
		matrix_render();
		ws2811_render(&ledstring);
    }

    ws2811_fini(&ledstring);

    return 0;
}
