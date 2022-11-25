/*
    Description: PIGPIO Test to blink LED connected to GPIO pin 23 every second
    Reference: Adapted from Python code: https://scispec.dev/2020/04/04/blink-led-with-python-and-pigpio-library-2/
*/

#include <stdio.h>
#include <pigpio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
   unsigned int GPIO = 23;
   unsigned int level = 0;

   if (gpioInitialise() < 0) return 1;

   while(1) {
        gpioWrite(GPIO, level);
        sleep(1);
        level=!level;
   }

   gpioTerminate();
}