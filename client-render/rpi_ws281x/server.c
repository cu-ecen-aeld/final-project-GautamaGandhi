#include <stdio.h>
#include "seq.h"

void initialize_camera();
void shutdown_camera();

// int main(int argc, char **argv)
// {
//     unsigned char *image_buffer;

//     initialize_camera();

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