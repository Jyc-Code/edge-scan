#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <linux/videodev2.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <sys/select.h>
#include <sys/time.h>
#include <pthread.h>

#include "print.h"
#include "lcd.h"
#include "ffmpeg.h"
#define DEV_PATH "/dev/video1"

extern pthread_mutex_t gGetYuyvMutex;

int fp = 0;
unsigned int i;
sBUFFER *buffers = NULL;

void v4l2_init(void)
{ 
    struct v4l2_capability cap;
    
    fp = open(DEV_PATH, O_RDWR);
    if (fp < 0) {
        ERR("Can't open camera!\r\n");
        exit(-1);
    }

    /* 读取设备信息 */
    if (ioctl(fp, VIDIOC_QUERYCAP, &cap)) {
        ERR("Read camera infomation failed!\r\n");
        exit(-1);
    } else {
       DEBUG("driver: %s\n", cap.driver);
       DEBUG("card: %s\n", cap.card);
       DEBUG("bus_info: %s\n", cap.bus_info);
       DEBUG("version: %d\n", cap.version);
       DEBUG("capability: %x\n", cap.capabilities);
    } 

    /* 查看设备是否支持视频捕获和流 */
    if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == V4L2_CAP_VIDEO_CAPTURE)
        DEBUG("Device %s support capture\r\n", DEV_PATH);
    else
        DEBUG("Device %s not support capture\r\n", DEV_PATH);    
    if ((cap.capabilities & V4L2_CAP_STREAMING) == V4L2_CAP_STREAMING)
        DEBUG("Device %s support streaming\r\n", DEV_PATH);
    else 
        DEBUG("Device %s not support streaming\r\n", DEV_PATH);

    
    struct v4l2_fmtdesc fmtdesc;

    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

#ifdef DEBUG
    DEBUG("Support format:\r\n");
    while (ioctl(fp, VIDIOC_ENUM_FMT, &fmtdesc) != -1) {
        DEBUG("%d.%s\r\n", fmtdesc.index + 1, fmtdesc.description);
        fmtdesc.index++;
    }
#endif

    /* 设置帧格式、检查 */
    struct v4l2_format format;

    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    format.fmt.pix.height = 480;
    format.fmt.pix.width = 640;
    format.fmt.pix.field = V4L2_FIELD_INTERLACED;

    if (ioctl(fp, VIDIOC_S_FMT, &format) == -1) {
        ERR("Unable to set format\r\n");
        exit(-1);
    }

    if (ioctl(fp, VIDIOC_G_FMT, &format) == -1) {
        ERR("Unable to get format\r\n");
        exit(-1);
    }
#ifdef DEBUG
    DEBUG("type:%d\r\n", format.type);
    /* DEBUG("pixelformat:%c%c%c%c\r\n", format.fmt.pix.pixelformat & 0xff); */
    DEBUG("height:%d\r\n", format.fmt.pix.height);
    DEBUG("width:%d\r\n", format.fmt.pix.width);
    DEBUG("field:%d\r\n", format.fmt.pix.field);
#endif
    /* 申请缓冲区 */
    struct v4l2_requestbuffers req_buf;
    struct v4l2_buffer buf;

    req_buf.count = 4;
    req_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req_buf.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fp, VIDIOC_REQBUFS, &req_buf)) {
        ERR("memory request failed!\r\n");
        exit(-1);
    }
    buffers = (sBUFFER *)malloc(req_buf.count * sizeof(sBUFFER));

    if (!buffers) {
        ERR("Out of memory\r\n");
        exit(-1);
    }

    for (i = 0;i < req_buf.count;i++) {
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(fp, VIDIOC_QUERYBUF, &buf)) {
            ERR("Query buffer error\r\n");
            exit(-1);
        }
        buffers[i].length = buf.length;
        buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fp, buf.m.offset);
        if (buffers[i].start == MAP_FAILED) {
            ERR("buffer map error\r\n");
            exit(-1);
        }
    }

    /* 入列 */
    for (i = 0;i < req_buf.count;i++) {
        buf.index = i;
        ioctl(fp, VIDIOC_QBUF, &buf);
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fp, VIDIOC_STREAMON, &type) == -1) {
            ERR("error to start!\r\n");
            exit(-1);
    } else ERR("start!\r\n");
}

sBUFFER* v4l2_get(void)
{
    struct v4l2_buffer buf;
    sBUFFER* buffer_get = NULL;

    pthread_mutex_lock(&gGetYuyvMutex);
    
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fp, VIDIOC_DQBUF, &buf) == -1) {
        ERR("get error\r\n");
        v4l2_close();
        exit(-1);
    }

    buffer_get = &buffers[buf.index];
    if (ioctl(fp, VIDIOC_QBUF, &buf) != 0) {
        ERR("error to put quene\r\n");
        exit(-1);
    }

    pthread_mutex_unlock(&gGetYuyvMutex);
    return buffer_get;
}

extern void* fbp;

AVFrame *Input_pFrame;
AVFrame *Output_pFrame;
struct SwsContext *img_ctx = NULL;
uint8_t* yuyv2rgb24_ffmpeg(uint8_t *pointer)
{
    uint8_t *rgb = (uint8_t *)malloc(640*480*4);
    int img_x = 640;
    int img_h = 480;
  
    Input_pFrame = av_frame_alloc();
    Output_pFrame = av_frame_alloc();
    
    img_ctx = sws_getContext(img_x, img_h, AV_PIX_FMT_YUYV422, img_x, img_h, AV_PIX_FMT_RGB32, SWS_FAST_BILINEAR, NULL, NULL, NULL);
    
    av_image_fill_arrays(Output_pFrame->data, Output_pFrame->linesize, rgb, AV_PIX_FMT_BGR32, img_x, img_h, 1);
    av_image_fill_arrays(Input_pFrame->data, Input_pFrame->linesize, pointer, AV_PIX_FMT_YUYV422, img_x, img_h, 1);
    
    DEBUG("InPut->linesize:%d\n Output->linesize:%d\n",*(Input_pFrame->linesize),*(Output_pFrame->linesize));

    sws_scale(img_ctx, Input_pFrame->data, Input_pFrame->linesize, 0, img_h, Output_pFrame->data, Output_pFrame->linesize);

    return (uint8_t *)Output_pFrame->data;
}

void resolutionChange(uint8_t *pointer, int row, int cols)
{
    Input_pFrame = av_frame_alloc();
    Output_pFrame = av_frame_alloc();

    img_ctx = sws_getContext(row, cols, AV_PIX_FMT_RGB32, row, cols, AV_PIX_FMT_RGB32, SWS_FAST_BILINEAR, NULL, NULL, NULL);

    av_image_fill_arrays(Output_pFrame->data, Output_pFrame->linesize, (const uint8_t *)fbp, AV_PIX_FMT_RGB32, 800, 480, 1);
    av_image_fill_arrays(Input_pFrame->data, Input_pFrame->linesize, pointer, AV_PIX_FMT_RGB32, row, cols, 1);

    sws_scale(img_ctx, Input_pFrame->data, Input_pFrame->linesize, 0, cols, Output_pFrame->data, Output_pFrame->linesize);
}

void v4l2_close(void)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (fp < 0) {
        ERR("close failed\r\n");
        exit(-1);
    }
    
    if (ioctl(fp, VIDIOC_STREAMOFF, &type) == -1) {
        ERR("close stream failed\r\n");
        exit(-1);
    }

    for (i = 0;i < 4;i++) {
        if (!munmap(buffers[i].start, buffers[i].length)) {
            ERR("error to munmap\r\n");
            exit(-1);
        }
    }

    /*if(rgb)free(rgb);*/
    if(Input_pFrame)av_free(Input_pFrame);
    if(Output_pFrame)av_free(Output_pFrame);
    if(img_ctx)sws_freeContext(img_ctx);

    free(buffers);
    close(fp);
    DEBUG("v4l2 close\r\n");
}
