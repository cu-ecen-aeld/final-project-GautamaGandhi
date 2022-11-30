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

// Test Image RGB render
unsigned char array[] = {126,101,94,96,67,67,71,45,49,70,46,51,69,50,58,52,
37,47,37,22,30,38,32,38,110,126,122,107,111,115,164,
179,185,198,209,211,190,201,206,88,91,108,47,38,52,57,
80,87,119,139,135,135,154,159,175,209,208,129,191,185,127,
164,171,98,82,101,46,49,66,81,122,132,117,131,129,145,
170,173,124,177,169,101,154,143,101,131,140,83,70,91,65,
87,101,87,144,158,106,120,112,116,133,129,117,150,144,110,
156,146,132,160,169,97,88,108,121,118,132,85,108,112,97,
119,104,98,110,106,98,123,117,128,154,155,122,136,163,53,
60,85,115,96,111,103,93,97,82,99,95,85,98,95,89,
122,112,109,122,124,52,60,84,7,25,51,41,38,51,102,
78,85,37,32,35,28,25,31,31,30,35,52,49,57,46,
46,56,19,22,31,8,7,11,27,19,22};

void check_array()
{
	int array_size = sizeof(array)/sizeof(array[0]);

	// printf("Array_size = %d\n", array_size);
}

uint32_t led_matrix[LED_COUNT];

void process_array(unsigned char *input_array, int array_size)
{
	for (int i = 0; i < 256; i++) {
		led_matrix[i] = (((input_array[3*i] << 5) / 256) << 16) | (((input_array[3*i+1]<< 5) / 256) << 8) | (((input_array[3*i+2]<< 5) / 256));
	}
}



// Test Image RGB render end



// Test Image START
uint8_t r8_pixels[] = { 252 ,251 ,  226 ,  227 ,  229 ,  243 ,  249 ,  252,
   255 ,  208 ,  123 ,  197 ,  171 ,  219 ,  255 ,  252,
   255 ,  206  , 164 ,  225 ,  187 ,   99 ,  205 ,  255,
   253  , 251  , 225 ,  207 ,  189 ,  201 ,  237 ,  254,
   252 ,  246 ,  207  ,  84  ,  86 ,  209 ,  247 ,  252,
   252  , 255  , 134  ,  32  ,  32  , 134  , 255 ,  252,
   255  , 230  ,  27  ,  70  ,  70  ,  27  , 230 ,  255,
   248  , 122  ,  73 ,  233 ,  233  ,  73  , 122  , 248,
    
};

uint8_t g8_pixels[] = {

    253 ,  249  ,  94  ,  24  ,  43  , 150  , 212 ,  255,
   255  , 194  ,  67  , 132 ,  117 ,  158 ,  226  , 254,
   255 ,  197 ,  127  , 173 ,  149 ,   85 ,  181  , 255,
   255  , 237 ,  123  , 106 ,   92 ,  105 ,  223  , 255,
   250 ,  111,    17  ,  13 ,   14 ,   19 ,  112   ,250,
   247 ,  175 ,   66  ,  33,    33 ,   66 ,  175  , 247,
   255 ,  208  ,  23  ,  71 ,   71 ,   23 ,  208 ,  255,
   248  ,  99  ,  43  , 231  , 231 ,   43 ,   99  , 248

};

uint8_t b8_pixels[] = {
    253 ,  249  , 100  ,  26  ,  42 ,  152  , 218  , 255,
   255  , 175  ,   5   , 10  ,  10  ,  50  , 170 ,  255,
   255  , 180  ,  21  ,   5  ,   8  ,  13  ,  99  , 251,
   254  , 243  ,  97 ,   58  ,  58  ,  93  , 230  , 255,
   249 ,  101  ,  55  , 173  , 173  ,  56  , 102  , 249,
   235  ,  20  , 124  , 219 ,  219  , 124  ,  20  , 235,
   245 ,  143  , 214  , 254 ,  254  , 214  , 143 ,  245,
   247  ,  83  ,  32 ,  235 ,  235  ,  32  ,  83  , 247
};


uint32_t rgb32_pixels[sizeof(r8_pixels)/sizeof(uint8_t)];

void test_image_render(void)
{
	int pixel_count = sizeof(r8_pixels)/sizeof(uint8_t);
    int index = 0;

    while(pixel_count){
		// uint16_t red_temp = (r8_pixels[index] << 5) / 256;
		// uint16_t green_temp = (g8_pixels[index] << 5) / 256;
		// uint16_t blue_temp = (b8_pixels[index] << 5) / 256;
		uint16_t red_temp = r8_pixels[index];// << 5) / 256;
		uint16_t green_temp = g8_pixels[index];
		uint16_t blue_temp = b8_pixels[index];
        rgb32_pixels[index] |= (red_temp << 16)| ( green_temp << 8 )| ( blue_temp << 0);
        index++;
        pixel_count--;
    }
}

// Test Image END

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

void matrix_raise(void)
{
    int x, y;

    for (y = 0; y < (height - 1); y++)
    {
        for (x = 0; x < width; x++)
        {
            // This is for the 8x8 Pimoroni Unicorn-HAT where the LEDS in subsequent
            // rows are arranged in opposite directions
            matrix[y * width + x] = matrix[(y + 1)*width + width - x - 1];
        }
    }
}

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

int dotspos[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
ws2811_led_t dotcolors[] =
{
    0x00200000,  // red
    0x00201000,  // orange
    0x00202000,  // yellow
    0x00002000,  // green
    0x00002020,  // lightblue
    0x00000020,  // blue
    0x00100010,  // purple
    0x00200010,  // pink
};

ws2811_led_t dotcolors_rgbw[] =
{
    0x00200000,  // red
    0x10200000,  // red + W
    0x00002000,  // green
    0x10002000,  // green + W
    0x00000020,  // blue
    0x10000020,  // blue + W
    0x00101010,  // white
    0x10101010,  // white + W

};

void matrix_bottom(void)
{
    int i;

    for (i = 0; i < (int)(ARRAY_SIZE(dotspos)); i++)
    {
        dotspos[i]++;
        if (dotspos[i] > (width - 1))
        {
            dotspos[i] = 0;
        }

        if (ledstring.channel[0].strip_type == SK6812_STRIP_RGBW) {
            matrix[dotspos[i] + (height - 1) * width] = dotcolors_rgbw[i];
        } else {
            matrix[dotspos[i] + (height - 1) * width] = dotcolors[i];
        }
    }
}

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

void fill_matrix_16x16(void)
{
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

static void ctrl_c_handler(int signum)
{
	(void)(signum);
    running = 0;
}

static void setup_handlers(void)
{
    struct sigaction sa =
    {
        .sa_handler = ctrl_c_handler,
    };

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}


void parseargs(int argc, char **argv, ws2811_t *ws2811)
{
	int index;
	int c;

	static struct option longopts[] =
	{
		{"help", no_argument, 0, 'h'},
		{"dma", required_argument, 0, 'd'},
		{"gpio", required_argument, 0, 'g'},
		{"invert", no_argument, 0, 'i'},
		{"clear", no_argument, 0, 'c'},
		{"strip", required_argument, 0, 's'},
		{"height", required_argument, 0, 'y'},
		{"width", required_argument, 0, 'x'},
		{"version", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};

	while (1)
	{

		index = 0;
		c = getopt_long(argc, argv, "cd:g:his:vx:y:", longopts, &index);

		if (c == -1)
			break;

		switch (c)
		{
		case 0:
			/* handle flag options (array's 3rd field non-0) */
			break;

		case 'h':
			fprintf(stderr, "%s version %s\n", argv[0], VERSION);
			fprintf(stderr, "Usage: %s \n"
				"-h (--help)    - this information\n"
				"-s (--strip)   - strip type - rgb, grb, gbr, rgbw\n"
				"-x (--width)   - matrix width (default 8)\n"
				"-y (--height)  - matrix height (default 8)\n"
				"-d (--dma)     - dma channel to use (default 10)\n"
				"-g (--gpio)    - GPIO to use\n"
				"                 If omitted, default is 18 (PWM0)\n"
				"-i (--invert)  - invert pin output (pulse LOW)\n"
				"-c (--clear)   - clear matrix on exit.\n"
				"-v (--version) - version information\n"
				, argv[0]);
			exit(-1);

		case 'D':
			break;

		case 'g':
			if (optarg) {
				int gpio = atoi(optarg);
/*
	PWM0, which can be set to use GPIOs 12, 18, 40, and 52.
	Only 12 (pin 32) and 18 (pin 12) are available on the B+/2B/3B
	PWM1 which can be set to use GPIOs 13, 19, 41, 45 and 53.
	Only 13 is available on the B+/2B/PiZero/3B, on pin 33
	PCM_DOUT, which can be set to use GPIOs 21 and 31.
	Only 21 is available on the B+/2B/PiZero/3B, on pin 40.
	SPI0-MOSI is available on GPIOs 10 and 38.
	Only GPIO 10 is available on all models.

	The library checks if the specified gpio is available
	on the specific model (from model B rev 1 till 3B)

*/
				ws2811->channel[0].gpionum = gpio;
			}
			break;

		case 'i':
			ws2811->channel[0].invert=1;
			break;

		case 'c':
			clear_on_exit=1;
			break;

		case 'd':
			if (optarg) {
				int dma = atoi(optarg);
				if (dma < 14) {
					ws2811->dmanum = dma;
				} else {
					printf ("invalid dma %d\n", dma);
					exit (-1);
				}
			}
			break;

		case 'y':
			if (optarg) {
				height = atoi(optarg);
				if (height > 0) {
					ws2811->channel[0].count = height * width;
				} else {
					printf ("invalid height %d\n", height);
					exit (-1);
				}
			}
			break;

		case 'x':
			if (optarg) {
				width = atoi(optarg);
				if (width > 0) {
					ws2811->channel[0].count = height * width;
				} else {
					printf ("invalid width %d\n", width);
					exit (-1);
				}
			}
			break;

		case 's':
			if (optarg) {
				if (!strncasecmp("rgb", optarg, 4)) {
					ws2811->channel[0].strip_type = WS2811_STRIP_RGB;
				}
				else if (!strncasecmp("rbg", optarg, 4)) {
					ws2811->channel[0].strip_type = WS2811_STRIP_RBG;
				}
				else if (!strncasecmp("grb", optarg, 4)) {
					ws2811->channel[0].strip_type = WS2811_STRIP_GRB;
				}
				else if (!strncasecmp("gbr", optarg, 4)) {
					ws2811->channel[0].strip_type = WS2811_STRIP_GBR;
				}
				else if (!strncasecmp("brg", optarg, 4)) {
					ws2811->channel[0].strip_type = WS2811_STRIP_BRG;
				}
				else if (!strncasecmp("bgr", optarg, 4)) {
					ws2811->channel[0].strip_type = WS2811_STRIP_BGR;
				}
				else if (!strncasecmp("rgbw", optarg, 4)) {
					ws2811->channel[0].strip_type = SK6812_STRIP_RGBW;
				}
				else if (!strncasecmp("grbw", optarg, 4)) {
					ws2811->channel[0].strip_type = SK6812_STRIP_GRBW;
				}
				else {
					printf ("invalid strip %s\n", optarg);
					exit (-1);
				}
			}
			break;

		case 'v':
			fprintf(stderr, "%s version %s\n", argv[0], VERSION);
			exit(-1);

		case '?':
			/* getopt_long already reported error? */
			exit(-1);

		default:
			exit(-1);
		}
	}
}


int main(int argc, char *argv[])
{

	printf("Socket Client\n");

	unsigned char *image_buffer;

    ws2811_return_t ret;

    sprintf(VERSION, "%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);

    parseargs(argc, argv, &ledstring);

    matrix = malloc(sizeof(ws2811_led_t) * width * height);

    setup_handlers();

    if ((ret = ws2811_init(&ledstring)) != WS2811_SUCCESS)
    {
        fprintf(stderr, "ws2811_init failed: %s\n", ws2811_get_return_t_str(ret));
        return ret;
    }

	test_image_render();

	check_array();

    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo; // Points to results

    memset(&hints, 0, sizeof(hints)); // Empty struct
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo("10.0.0.143", "9000", &hints, &servinfo)) != 0)
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

    unsigned char buffer[768];

    int client_fd = connect(socketfd, servinfo->ai_addr, servinfo->ai_addrlen);


    while (running)
    {
    
        int bytes_received = recv(socketfd, buffer, sizeof(buffer), 0);

		if (bytes_received == 768) {
			process_array(buffer, 768);

			fill_matrix_16x16();
			matrix_render();

			if ((ret = ws2811_render(&ledstring)) != WS2811_SUCCESS)
			{
				fprintf(stderr, "ws2811_render failed: %s\n", ws2811_get_return_t_str(ret));
				break;
			}

			// 15 frames /sec
			// usleep(1000000 / 15);

		}

        // t1 = time(0);
        // printf("Frame Received Bytes Received -> %d, latency -> %lf \n", bytes_received,difftime(t1, t0) * 1000);
    }
    printf("\n");

    freeaddrinfo(servinfo);

	

    // while (running)
    // {
	// 	//image_buffer = get_processed_image_data();
	// 	// usleep(10000);
	// 	process_array(image_buffer, 768);
	// 	// usleep(100000);
    //     // matrix_raise();
	//     fill_matrix_16x16();
    //     // matrix_bottom();
    //     matrix_render();

    //     if ((ret = ws2811_render(&ledstring)) != WS2811_SUCCESS)
    //     {
    //         fprintf(stderr, "ws2811_render failed: %s\n", ws2811_get_return_t_str(ret));
    //         break;
    //     }

    //     // 15 frames /sec
    //     usleep(1000000 / 15);
    // }

    if (clear_on_exit) {
		matrix_clear();
		matrix_render();
		ws2811_render(&ledstring);
    }

    ws2811_fini(&ledstring);

    printf ("\n");
    return ret;
}
