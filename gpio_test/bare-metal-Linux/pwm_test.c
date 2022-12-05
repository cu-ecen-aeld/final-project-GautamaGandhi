/*
 * gpio.h, clk.h, pwm.h
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
 * Name: pwm_test.c
 * Description: This file contains the test for PWM on the Raspberry Pi model 4B. This code has been sourced from the https://github.com/jgarff/rpi_ws281x
 *              library and modified to show the usage of PWM using the mmap function.
 * 
*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>
#include <sys/stat.h>
#include <sys/time.h>

#define RPI_OSC_FREQ      (54000000)   // Frequency of Raspberry Pi model 4B oscillator

// The following are the respective physical address offsets for the peripherals found in the following data sheet: 
// https://datasheets.raspberrypi.com/bcm2711/bcm2711-peripherals.pdf
#define RPI_PERIPH_OFFSET (0xfe000000)
#define GPIO_OFFSET       (0xfe200000)
#define PWM_OFFSET        (0xfe20c000)

/*
 * PWM and PCM clock offsets from https://www.scribd.com/doc/127599939/BCM2835-Audio-clocks
 *
 */
#define CM_PWM_OFFSET     (0xfe1010a0)

// The SoC used is BCM 2711
// Structure that maps the exact layout for the GPIO peripheral registers on the Raspberry Pi
typedef struct
{
    uint32_t fsel[6];                            // GPIO Function Select
    uint32_t resvd_0x18;
    uint32_t set[2];                             // GPIO Pin Output Set
    uint32_t resvd_0x24;
    uint32_t clr[2];                             // GPIO Pin Output Clear
    uint32_t resvd_0x30;
    uint32_t lev[2];                             // GPIO Pin Level
    uint32_t resvd_0x3c;
    uint32_t eds[2];                             // GPIO Pin Event Detect Status
    uint32_t resvd_0x48;
    uint32_t ren[2];                             // GPIO Pin Rising Edge Detect Enable
    uint32_t resvd_0x54;
    uint32_t fen[2];                             // GPIO Pin Falling Edge Detect Enable
    uint32_t resvd_0x60;
    uint32_t hen[2];                             // GPIO Pin High Detect Enable
    uint32_t resvd_0x6c;
    uint32_t len[2];                             // GPIO Pin Low Detect Enable
    uint32_t resvd_0x78;
    uint32_t aren[2];                            // GPIO Pin Async Rising Edge Detect
    uint32_t resvd_0x84;
    uint32_t afen[2];                            // GPIO Pin Async Falling Edge Detect
    uint32_t resvd_0x90;
    uint32_t pud;                                // GPIO Pin Pull up/down Enable
    uint32_t pudclk[2];                          // GPIO Pin Pull up/down Enable Clock
    uint32_t resvd_0xa0[4];
    uint32_t test;
} __attribute__((packed, aligned(4))) gpio_t;

// Structure that maps the layout for the PWM peripheral registers on the Raspberry Pi
typedef struct
{
    uint32_t ctl;
#define RPI_PWM_CTL_MSEN2                        (1 << 15)
#define RPI_PWM_CTL_USEF2                        (1 << 13)
#define RPI_PWM_CTL_POLA2                        (1 << 12)
#define RPI_PWM_CTL_SBIT2                        (1 << 11)
#define RPI_PWM_CTL_RPTL2                        (1 << 10)
#define RPI_PWM_CTL_MODE2                        (1 << 9)
#define RPI_PWM_CTL_PWEN2                        (1 << 8)
#define RPI_PWM_CTL_MSEN1                        (1 << 7)
#define RPI_PWM_CTL_CLRF1                        (1 << 6)
#define RPI_PWM_CTL_USEF1                        (1 << 5)
#define RPI_PWM_CTL_POLA1                        (1 << 4)
#define RPI_PWM_CTL_SBIT1                        (1 << 3)
#define RPI_PWM_CTL_RPTL1                        (1 << 2)
#define RPI_PWM_CTL_MODE1                        (1 << 1)
#define RPI_PWM_CTL_PWEN1                        (1 << 0)
    uint32_t sta;
#define RPI_PWM_STA_STA4                         (1 << 12)
#define RPI_PWM_STA_STA3                         (1 << 11)
#define RPI_PWM_STA_STA2                         (1 << 10)
#define RPI_PWM_STA_STA1                         (1 << 9)
#define RPI_PWM_STA_BERR                         (1 << 8)
#define RPI_PWM_STA_GAP04                        (1 << 7)
#define RPI_PWM_STA_GAP03                        (1 << 6)
#define RPI_PWM_STA_GAP02                        (1 << 5)
#define RPI_PWM_STA_GAP01                        (1 << 4)
#define RPI_PWM_STA_RERR1                        (1 << 3)
#define RPI_PWM_STA_WERR1                        (1 << 2)
#define RPI_PWM_STA_EMPT1                        (1 << 1)
#define RPI_PWM_STA_FULL1                        (1 << 0)
    uint32_t dmac;
#define RPI_PWM_DMAC_ENAB                        (1 << 31)
#define RPI_PWM_DMAC_PANIC(val)                  ((val & 0xff) << 8)
#define RPI_PWM_DMAC_DREQ(val)                   ((val & 0xff) << 0)
    uint32_t resvd_0x0c;
    uint32_t rng1;
    uint32_t dat1;
    uint32_t fif1;
    uint32_t resvd_0x1c;
    uint32_t rng2;
    uint32_t dat2;
} __attribute__((packed, aligned(4))) pwm_t;

// Structure that maps the layout for the Clock Management peripheral registers on the Raspberry Pi
typedef struct {
    uint32_t ctl;
#define CM_CLK_CTL_PASSWD                        (0x5a << 24)
#define CM_CLK_CTL_MASH(val)                     ((val & 0x3) << 9)
#define CM_CLK_CTL_FLIP                          (1 << 8)
#define CM_CLK_CTL_BUSY                          (1 << 7)
#define CM_CLK_CTL_KILL                          (1 << 5)
#define CM_CLK_CTL_ENAB                          (1 << 4)
#define CM_CLK_CTL_SRC_GND                       (0 << 0)
#define CM_CLK_CTL_SRC_OSC                       (1 << 0)
#define CM_CLK_CTL_SRC_TSTDBG0                   (2 << 0)
#define CM_CLK_CTL_SRC_TSTDBG1                   (3 << 0)
#define CM_CLK_CTL_SRC_PLLA                      (4 << 0)
#define CM_CLK_CTL_SRC_PLLC                      (5 << 0)
#define CM_CLK_CTL_SRC_PLLD                      (6 << 0)
#define CM_CLK_CTL_SRC_HDMIAUX                   (7 << 0)
    uint32_t div;
#define CM_CLK_DIV_PASSWD                        (0x5a << 24)
#define CM_CLK_DIV_DIVI(val)                     ((val & 0xfff) << 12)
#define CM_CLK_DIV_DIVF(val)                     ((val & 0xfff) << 0)
} __attribute__((packed, aligned(4))) cm_clk_t;

// mapmem function leveraged from the LED matrix library;
// mmap function call is used to map physical address to virtual memory
void *mapmem(uint32_t base, uint32_t size, const char *mem_dev) {
    uint32_t pagemask = ~0UL ^ (getpagesize() - 1);
    uint32_t offsetmask = getpagesize() - 1;
    int mem_fd;
    void *mem;

    mem_fd = open(mem_dev, O_RDWR | O_SYNC);
    if (mem_fd < 0) {
       perror("Can't open /dev/mem");
       return NULL;
    }

    mem = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, base & pagemask);
    if (mem == MAP_FAILED) {
        perror("mmap error\n");
        return NULL;
    }

    close(mem_fd);

    return (char *)mem + (base & offsetmask);
}

int main()
{
    // Mapping GPIO
    volatile gpio_t *gpio = mapmem(GPIO_OFFSET, sizeof(gpio_t), "/dev/mem");
    printf("Device->GPIO address: %p\n", gpio);

    if (!gpio)
    {
        printf("Failed to map GPIO\n");
    }

    // Below lines can be uncommented to set the GPIO Pin 23 high or low
    // Set LED ON
    // gpio->fsel[2] &= ~(0x7 << 9); // Pin 23
    // gpio->fsel[2] |= (0x1 << 9); // Pin 23
    // gpio->clr[0] |= (1 << 23); // Set the pin
    // gpio->set[0] |= (1 << 23); // Set the pin

    // Mapping PWM
    volatile pwm_t *pwm = mapmem(PWM_OFFSET, sizeof(pwm_t), "/dev/mem");
    printf("Device->PWM address: %p\n", pwm);

    if (!pwm)
    {
        printf("Failed to map PWM\n");
    }

    // Mapping Clock for PWM
    volatile cm_clk_t *cm_clk = mapmem(CM_PWM_OFFSET, sizeof(cm_clk_t), "/dev/mem");

    // Turn off the PWM in case already running
    pwm->ctl = 0;
    usleep(10);

    // Stopping the clock if its running
    cm_clk->ctl = CM_CLK_CTL_PASSWD | CM_CLK_CTL_KILL;
    usleep(10);
    while (cm_clk->ctl & CM_CLK_CTL_BUSY);

    uint32_t osc_freq = RPI_OSC_FREQ;

    uint32_t freq = 800000;

    // Setup the Clock - Use OSC @ 19.2Mhz w/ 3 clocks/tick
    cm_clk->div = CM_CLK_DIV_PASSWD | CM_CLK_DIV_DIVI(osc_freq / (3 * freq));
    cm_clk->ctl = CM_CLK_CTL_PASSWD | CM_CLK_CTL_SRC_OSC;
    cm_clk->ctl = CM_CLK_CTL_PASSWD | CM_CLK_CTL_SRC_OSC | CM_CLK_CTL_ENAB;
    usleep(10);
    while (!(cm_clk->ctl & CM_CLK_CTL_BUSY));

    // Selecting Alternate Function 5 for GPIO Pin 18
    gpio->fsel[1] &= ~(0x7 << 24);
    gpio->fsel[1] |= (0x2 << 24);

    // PWM range is set to 50
    pwm->rng1 = 50;
    usleep(10);
    // Enabling PWM
    pwm->ctl |= RPI_PWM_CTL_PWEN1 | RPI_PWM_CTL_PWEN2;
    usleep(10);
    // DAT1 is the data register for the PWM signal
    pwm->dat1 = 0;

    // While loop that increases and then decreases the PWM duty cycle
    // This pin is connected to an LED and the effect can be viewed as brightening/dimming the LED
    while (1)
    {
        for (int i = 0; i < 10; i++){
            pwm->dat1 += 5;
            usleep(200000);
        }

         for (int i = 0; i < 10; i++){
            pwm->dat1 -= 5;
            usleep(200000);
        }

    }

    return 0;
}