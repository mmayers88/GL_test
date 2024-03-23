#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <memory>
#include <semaphore> 

#include <jpeglib.h>
using namespace std; 
  
mutex m;

#define IMAGE_WIDTH_720P    1280
#define IMAGE_HEIGHT_720P   720

#define IMAGE_WIDTH_480P    640
#define IMAGE_HEIGHT_480P   480

int image_width = IMAGE_WIDTH_480P;
int image_height = IMAGE_HEIGHT_480P;

uint8_t *buffer;
int uniBufLength = 0;
int phase = 0;

static int xioctl(int fd, int request, void *arg)
{
    int r;

    do 
    {
        r = ioctl(fd, request, arg);
    }
    while (-1 == r && EINTR == errno);

    return r;
}

int print_caps(int fd, int pixel_format)
{
    struct v4l2_capability caps = {};
    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &caps))
    {
        perror("Querying Capabilities");
        return 1;
    }

    printf( "Driver Caps:\n"
            "  Driver: \"%s\"\n"
            "  Card: \"%s\"\n"
            "  Bus: \"%s\"\n"
            "  Version: %d.%d\n"
            "  Capabilities: %08x\n",
            caps.driver,
            caps.card,
            caps.bus_info,
            (caps.version>>16)&&0xff,
            (caps.version>>24)&&0xff,
            caps.capabilities);

    int support_grbg10 = 0;

    struct v4l2_fmtdesc fmtdesc = {0};
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    char fourcc[5] = {0};
    char c, e;
    printf("  FMT : CE Desc\n--------------------\n");
    while (0 == xioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc))
    {
        strncpy(fourcc, (char *)&fmtdesc.pixelformat, 4);
        if (fmtdesc.pixelformat == V4L2_PIX_FMT_SGRBG10)
            support_grbg10 = 1;
        c = fmtdesc.flags & 1? 'C' : ' ';
        e = fmtdesc.flags & 2? 'E' : ' ';
        printf("  %s: %c%c %s\n", fourcc, c, e, fmtdesc.description);
        fmtdesc.index++;
    }

    struct v4l2_format fmt = {0};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = image_width;
    fmt.fmt.pix.height = image_height;

    if(pixel_format == 0) fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    if(pixel_format == 1) fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    if(pixel_format == 2) fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;

    fmt.fmt.pix.field = V4L2_FIELD_NONE;
    
    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
    {
        perror("Setting Pixel Format");
        return 1;
    }

    strncpy(fourcc, (char *)&fmt.fmt.pix.pixelformat, 4);
    printf( "Selected Camera Mode:\n"
            "  Width: %d\n"
            "  Height: %d\n"
            "  PixFmt: %s\n"
            "  Field: %d\n",
            fmt.fmt.pix.width,
            fmt.fmt.pix.height,
            fourcc,
            fmt.fmt.pix.field);
    return 0;
}
 
int init_mmap(int fd)
{
    struct v4l2_requestbuffers req = {0};
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
 
    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req))
    {
        perror("Requesting Buffer");
        return 1;
    }
 
    struct v4l2_buffer buf = {0};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    if(-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
    {
        perror("Querying Buffer");
        return 1;
    }
    uniBufLength =  buf.length;
    buffer = (uint8_t*) mmap (NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    printf("Length: %d\nAddress: %p\n", buf.length, buffer);
    // printf("Image Length: %d\n", buf.bytesused);
    
    return 0;
}

int capture_image(int fd, int format)
{
    struct v4l2_buffer buf = {0};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;

    if(-1 == xioctl(fd, VIDIOC_QBUF, &buf))
    {
        perror("Query Buffer");
        return 1;
    }
 
    if(-1 == xioctl(fd, VIDIOC_STREAMON, &buf.type))
    {
        perror("Start Capture");
        return 1;
    }
 
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    struct timeval tv = {0};
    tv.tv_sec = 2;

    int r = select(fd+1, &fds, NULL, NULL, &tv);
    if(-1 == r)
    {
        perror("Waiting for Frame");
        return 1;
    }
 
    if(-1 == xioctl(fd, VIDIOC_DQBUF, &buf))
    {
        perror("Retrieving Frame");
        return 1;
    }
    return 0;
}

int clache(int format)
{
    std::vector<int> hist(256, 0);   
    for(int i = 0; i < uniBufLength; i+=4)
    {
        hist[buffer[i]]++;
        hist[buffer[i+2]]++;
    }

    int i = 0;
    int total = image_height * image_width;
    while (!hist[i]) ++i;

    // Compute scale
    float scale = (256 - 1.f) / (total - hist[i]);

     // Initialize lut
    std::vector<int> lut(256, 0);
    i++;

    int sum = 0;
    for (; i < hist.size(); ++i) {
        sum += hist[i];
        // the value is saturated in range [0, max_val]
        lut[i] = std::max(0, std::min(int(round(sum * scale)), 255));
    }

    // Apply equalization
    for (int i = 0; i < uniBufLength; i+=4) {
        buffer[i] = lut[buffer[i]];
        buffer[i+2] = lut[buffer[i+2]];
        //buffer[i+1] = 128;
        //buffer[i+3] = 128;
    }
    return 0;
}

void disp(int format)
{
    cv::Mat cv_img;

    if (format == 0 ){
        // Decode YUYV
        cv::Mat img = cv::Mat(cv::Size(image_width, image_height), CV_8UC2, buffer);
        cv::cvtColor(img, cv_img, cv::COLOR_YUV2RGB_YVYU);
    }

    if (format == 1) {
        // Decode MJPEG
        cv::_InputArray pic_arr(buffer, image_width * image_height * 3);
        cv_img = cv::imdecode(pic_arr, cv::IMREAD_UNCHANGED);
    }

    if (format == 2) {
        // Decode RGB3
        cv_img = cv::Mat(cv::Size(image_width, image_height), CV_8UC3, buffer);
    }

    cv::imshow("view", cv_img);

    cv::waitKey(1);
}

void workerCap(int fd, int format)
{
    
    while(1)
    {
    if(phase == 2)
    {
        m.lock();
        capture_image(fd, format);
        phase = 0;
        m.unlock();
        this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    }
}
void workerEnh(int format)
{
    
    while(1)
    {
        if(phase == 0)
        {
            m.lock();
            clache(format);
            phase = 1;
            m.unlock();
            this_thread::sleep_for(std::chrono::milliseconds(3));
        }
    }
}
void workerDisp(int format)
{
    while(1)
    {
    if(phase == 1)
    {
        m.lock();
        disp(format);
        phase = 2;
        m.unlock();
        this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    }
}


int main()
{
    // Boiler plate camera setup
    int fd, opt;

    int pixel_format = 0;     /* V4L2_PIX_FMT_YUYV */
    char *v4l2_devname = "/dev/video0";

    fd = open(v4l2_devname, O_RDWR);
    if (fd == -1)
    {
        perror("Opening video device");
        return 1;
    }

    if(print_caps(fd, pixel_format))
    {
        return 1;
    }
    
    if(init_mmap(fd))
    {
        return 1;
    }

    thread captureThread(workerCap, fd, pixel_format);
    thread enhancementThread(workerEnh, pixel_format);
    thread dispThread(workerDisp, pixel_format);

    while(1)
    {
    
    }
}