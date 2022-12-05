/***************************************************************************
 * AESD Final Project
 * Author: Starter code by Sam Siewert(RTES), Modifications + new code by Chinmay Shalawadi 
 * Institution: University of Colorado Boulder
 * Mail id: chsh1552@colorado.edu
 * References: Wikipedia, ChatGPT & stb header library
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h> 
#include <fcntl.h> 
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "seq.h"

#define RGB

// Switches for code flow
// #define GRB

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"
#include <linux/videodev2.h>
#include <time.h>
#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define COLOR_CONVERT
#define HRES 320
#define VRES 240

//Changes to make image look better on matrix
#define LUMINENCE_OFFSET (30)
#define LUMINENCE_THRESHOLD (125)

// Format is used by a number of functions, so made as a file global
static struct v4l2_format fmt;
struct buffer
{
    void *start;
    size_t length;
};
char *dev_name = "/dev/video0";
static int fd = -1;
struct buffer *buffers;
static unsigned int n_buffers;
static int out_buf;
static int force_format = 1;
static int frame_count = 10;
unsigned int framecnt = 0;
unsigned char *bigbuffer;
unsigned char *bigbuffer2;

//-------------------------------------errno_exit--------------------------------------------------------------------------
static void errno_exit(const char *s)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    exit(EXIT_FAILURE);
}
//-------------------------------------xioctl--------------------------------------------------------------------------
static int xioctl(int fh, int request, void *arg)
{
    int r;
    do
    {
        r = ioctl(fh, request, arg);

    } while (-1 == r && EINTR == errno);
    return r;
}
//-------------------------------------yuv2rgb--------------------------------------------------------------------------
//
//This code has been modified for color adjustments for LED matrix
//
// This is probably the most acceptable conversion from camera YUYV to RGB
//
// Wikipedia has a good discussion on the details of various conversions and cites good references:
// http://en.wikipedia.org/wiki/YUV
//
// Also http://www.fourcc.org/yuv.php
//
// What's not clear without knowing more about the camera in question is how often U & V are sampled compared
// to Y.
//
// E.g. YUV444, which is equivalent to RGB, where both require 3 bytes for each pixel
//      YUV422, which we assume here, where there are 2 bytes for each pixel, with two Y samples for one U & V,
//              or as the name implies, 4Y and 2 UV pairs
//      YUV420, where for every 4 Ys, there is a single UV pair, 1.5 bytes for each pixel or 36 bytes for 24 pixels

void yuv2rgb(int y, int u, int v, unsigned char *r, unsigned char *g, unsigned char *b)
{
    int r1, g1, b1, c;

    // replaces floating point coefficients
    if (LUMINENCE_OFFSET && y > LUMINENCE_THRESHOLD)
        c = y - (16 + LUMINENCE_OFFSET);
    else
        c = y - 16;

    int d = u - 128, e = v - 128;

    // Conversion that avoids floating point
    r1 = (298 * c + 409 * e + 128) >> 8;
    g1 = (298 * c - 100 * d - 208 * e + 128) >> 8;
    b1 = (298 * c + 516 * d + 128) >> 8;

    // Computed values may need clipping.
    if (r1 > 255)
        r1 = 255;
    if (g1 > 255)
        g1 = 255;
    if (b1 > 255)
        b1 = 255;

    if (r1 < 0)
        r1 = 0;
    if (g1 < 0)
        g1 = 0;
    if (b1 < 0)
        b1 = 0;

#ifdef RGB
    *r = r1;
    *g = g1;
    *b = b1;
#endif

#ifdef GRB
    *r = g1;
    *g = r1;
    *b = b1;
#endif
}
//-------------------------------------process_image--------------------------------------------------------------------------
static void process_image(const void *p, int size)
{
    int i, newi, newsize = 0;
    int y_temp, y2_temp, u_temp, v_temp;
    unsigned char *pptr = (unsigned char *)p;
    if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV)
    {
         for (i = 0, newi = 0; i < size; i = i + 4, newi = newi + 6)
        {
            y_temp = (int)pptr[i];
            u_temp = (int)pptr[i + 1];
            y2_temp = (int)pptr[i + 2];
            v_temp = (int)pptr[i + 3];
            yuv2rgb(y_temp, u_temp, v_temp, &bigbuffer[newi], &bigbuffer[newi + 1], &bigbuffer[newi + 2]);
            yuv2rgb(y2_temp, u_temp, v_temp, &bigbuffer[newi + 3], &bigbuffer[newi + 4], &bigbuffer[newi + 5]);
        }
        // crop and resize the image down to 16x16
        stbir_resize_uint8(bigbuffer, 320, 240, 0, bigbuffer2, 16, 16, 0, 3);
    }

    else
    {
        printf("ERROR - unknown dump format\n");
    }

    fflush(stderr);
    fflush(stdout);
}
//-------------------------------------read_frame--------------------------------------------------------------------------
static int read_frame(void)
{
    struct v4l2_buffer buf;
    unsigned int i;

    CLEAR(buf);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf))
    {
        switch (errno)
        {
        case EAGAIN:
            return 0;

        case EIO:
            /* Could ignore EIO, but drivers should only set for serious errors, although some set for
               non-fatal errors too.
             */
            return 0;

        default:
            printf("mmap failure\n");
            errno_exit("VIDIOC_DQBUF");
        }
    }

    assert(buf.index < n_buffers);

    process_image(buffers[buf.index].start, buf.bytesused);

    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
        errno_exit("VIDIOC_QBUF");

    // printf("R");
    return 1;
}
//-------------------------------------take_picture--------------------------------------------------------------------------
void take_picture(void)
{

    for (;;)
    {
        fd_set fds;
        struct timeval tv;
        int r;

        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        /* Timeout. */
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        r = select(fd + 1, &fds, NULL, NULL, &tv);

        if (-1 == r)
        {
            if (EINTR == errno)
                continue;
            errno_exit("select");
        }

        if (0 == r)
        {
            fprintf(stderr, "select timeout\n");
            exit(EXIT_FAILURE);
        }

        if (read_frame())
        {
            break;
        }
    }
}
//-------------------------------------get_processed_image--------------------------------------------------------------------------
unsigned char *get_processed_image_data()
{
    take_picture();
    return bigbuffer2;
}
//-------------------------------------stop_capturing--------------------------------------------------------------------------
void stop_capturing(void)
{
    enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
        errno_exit("VIDIOC_STREAMOFF");

    free(bigbuffer);
    free(bigbuffer2);
}
//-------------------------------------start_capturing--------------------------------------------------------------------------
void start_capturing(void)
{
    unsigned int i;
    enum v4l2_buf_type type;

    for (i = 0; i < n_buffers; ++i)
    {
        struct v4l2_buffer buf;
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
            errno_exit("VIDIOC_QBUF");
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
        errno_exit("VIDIOC_STREAMON");

    // Allocating memories for the buffers
    bigbuffer = malloc(sizeof(uint8_t) * 320 * 240 * 3);
    bigbuffer2 = malloc(sizeof(uint8_t) * 768);
}
//-------------------------------------uninit_device--------------------------------------------------------------------------
void uninit_device(void)
{
    unsigned int i;

    for (i = 0; i < n_buffers; ++i)
        if (-1 == munmap(buffers[i].start, buffers[i].length))
            errno_exit("munmap");

    free(buffers);
}
//-------------------------------------init_mmap--------------------------------------------------------------------------
static void init_mmap(void)
{
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count = 6;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req))
    {
        if (EINVAL == errno)
        {
            fprintf(stderr, "%s does not support "
                            "memory mapping\n",
                    dev_name);
            exit(EXIT_FAILURE);
        }
        else
        {
            errno_exit("VIDIOC_REQBUFS");
        }
    }

    if (req.count < 2)
    {
        fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
        exit(EXIT_FAILURE);
    }

    buffers = calloc(req.count, sizeof(*buffers));

    if (!buffers)
    {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers)
    {
        struct v4l2_buffer buf;

        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;

        if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
            errno_exit("VIDIOC_QUERYBUF");

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start =
            mmap(NULL /* start anywhere */,
                 buf.length,
                 PROT_READ | PROT_WRITE /* required */,
                 MAP_SHARED /* recommended */,
                 fd, buf.m.offset);

        if (MAP_FAILED == buffers[n_buffers].start)
            errno_exit("mmap");
    }
}
//-------------------------------------init_device--------------------------------------------------------------------------
void init_device(void)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    unsigned int min;

    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap))
    {
        if (EINVAL == errno)
        {
            fprintf(stderr, "%s is no V4L2 device\n",
                    dev_name);
            exit(EXIT_FAILURE);
        }
        else
        {
            errno_exit("VIDIOC_QUERYCAP");
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        fprintf(stderr, "%s is no video capture device\n",
                dev_name);
        exit(EXIT_FAILURE);
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING))
    {
        fprintf(stderr, "%s does not support streaming i/o\n",
                dev_name);
        exit(EXIT_FAILURE);
    }

    /* Select video input, video standard and tune here. */

    CLEAR(cropcap);

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap))
    {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */

        if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop))
        {
            switch (errno)
            {
            case EINVAL:
                /* Cropping not supported. */
                break;
            default:
                /* Errors ignored. */
                break;
            }
        }
    }

    CLEAR(fmt);

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (force_format)
    {
        fmt.fmt.pix.width = HRES;
        fmt.fmt.pix.height = VRES;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        fmt.fmt.pix.field = V4L2_FIELD_NONE;

        if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
            errno_exit("VIDIOC_S_FMT");
    }
    else
    {
        printf("ASSUMING FORMAT\n");
        /* Preserve original settings as set by v4l2-ctl for example */
        if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt))
            errno_exit("VIDIOC_G_FMT");
    }

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;

    init_mmap();
}
//-------------------------------------close_device--------------------------------------------------------------------------
void close_device(void)
{
    if (-1 == close(fd))
        errno_exit("close");

    fd = -1;
}
//-------------------------------------open_device--------------------------------------------------------------------------
void open_device(void)
{
    struct stat st;

    if (-1 == stat(dev_name, &st))
    {
        fprintf(stderr, "Cannot identify '%s': %d, %s\n",
                dev_name, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (!S_ISCHR(st.st_mode))
    {
        fprintf(stderr, "%s is no device\n", dev_name);
        exit(EXIT_FAILURE);
    }

    fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

    if (-1 == fd)
    {
        fprintf(stderr, "Cannot open '%s': %d, %s\n",
                dev_name, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
}
//-------------------------------------End--------------------------------------------------------------------------
