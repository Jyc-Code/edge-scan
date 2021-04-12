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

#include "lcd.h"
#include "ffmpeg.h"
#define DEV_PATH "/dev/video1"

int fp = 0;
unsigned int i;
struct buffer *buffers = NULL;

void v4l2_init(void)
{ 
    struct v4l2_capability cap;
    
    fp = open(DEV_PATH, O_RDWR);
    if (fp < 0) {
        printf("Can't open camera!\r\n");
        exit(-1);
    }

    /* 读取设备信息 */
    if (ioctl(fp, VIDIOC_QUERYCAP, &cap)) {
        printf("Read camera infomation failed!\r\n");
        exit(-1);
    } else {
       printf("driver: %s\n", cap.driver);
       printf("card: %s\n", cap.card);
       printf("bus_info: %s\n", cap.bus_info);
       printf("version: %d\n", cap.version);
       printf("capability: %x\n", cap.capabilities);
    } 

    /* 查看设备是否支持视频捕获和流 */
    if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == V4L2_CAP_VIDEO_CAPTURE)
        printf("Device %s support capture\r\n", DEV_PATH);
    else
        printf("Device %s not support capture\r\n", DEV_PATH);    
    if ((cap.capabilities & V4L2_CAP_STREAMING) == V4L2_CAP_STREAMING)
        printf("Device %s support streaming\r\n", DEV_PATH);
    else 
        printf("Device %s not support streaming\r\n", DEV_PATH);

    
    struct v4l2_fmtdesc fmtdesc;

    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

#ifdef DEBUG
    printf("Support format:\r\n");
    while (ioctl(fp, VIDIOC_ENUM_FMT, &fmtdesc) != -1) {
        printf("%d.%s\r\n", fmtdesc.index + 1, fmtdesc.description);
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
        printf("Unable to set format\r\n");
        exit(-1);
    }

    if (ioctl(fp, VIDIOC_G_FMT, &format) == -1) {
        printf("Unable to get format\r\n");
        exit(-1);
    }
#ifdef DEBUG
    printf("type:%d\r\n", format.type);
    printf("pixelformat:%c%c%c%c\r\n", format.fmt.pix.pixelformat & 0xff);
    printf("height:%d\r\n", format.fmt.pix.height);
    printf("width:%d\r\n", format.fmt.pix.width);
    printf("field:%d\r\n", format.fmt.pix.field);
#endif
    /* 申请缓冲区 */
    struct v4l2_requestbuffers req_buf;
    struct v4l2_buffer buf;

    req_buf.count = 4;
    req_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req_buf.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fp, VIDIOC_REQBUFS, &req_buf)) {
        printf("memory request failed!\r\n");
        exit(-1);
    }
    buffers = (struct buffer *)malloc(req_buf.count * sizeof(struct buffer));

    if (!buffers) {
        printf("Out of memory\r\n");
        exit(-1);
    }

    for (i = 0;i < req_buf.count;i++) {
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(fp, VIDIOC_QUERYBUF, &buf)) {
            printf("Query buffer error\r\n");
            exit(-1);
        }
        buffers[i].length = buf.length;
        buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fp, buf.m.offset);
        if (buffers[i].start == MAP_FAILED) {
            printf("buffer map error\r\n");
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
            printf("error to start!\r\n");
            exit(-1);
    } else printf("start!\r\n");
}

struct buffer* v4l2_get(void)
{
    struct v4l2_buffer buf;
    struct buffer* buffer_get = NULL;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fp, VIDIOC_DQBUF, &buf) == -1) {
        printf("get error\r\n");
        v4l2_close();
        exit(-1);
    }

    buffer_get = &buffers[buf.index];
    if (ioctl(fp, VIDIOC_QBUF, &buf) != 0) {
        printf("error to put quene\r\n");
        exit(-1);
    }
    return buffer_get;
}

extern void* fbp;

/*unsigned char *rgb;*/
AVFrame *Input_pFrame;
AVFrame *Output_pFrame;
struct SwsContext *img_ctx = NULL;
/* and display */
AVFrame* yuvv_2_rgb24_ffmpeg(unsigned char *pointer)
{
    /*rgb = (unsigned char *)malloc(800*480*4);*/
    int img_x = 640;
    int img_h = 480;
  
    Input_pFrame = av_frame_alloc();
    Output_pFrame = av_frame_alloc();

    img_ctx = sws_getContext(img_x, img_h, AV_PIX_FMT_YUYV422, img_x, img_h, AV_PIX_FMT_RGB32, SWS_FAST_BILINEAR, NULL, NULL, NULL);

    avpicture_fill((AVPicture*)Output_pFrame, (uint8_t const *)fbp, AV_PIX_FMT_RGB32, 800, img_h);
    avpicture_fill((AVPicture*)Input_pFrame, pointer, AV_PIX_FMT_YUYV422, img_x, img_h);

    sws_scale(img_ctx, Input_pFrame->data, Input_pFrame->linesize, 0, img_h, Output_pFrame->data, Output_pFrame->linesize);
	/*saveBmp(fbp);*/

	/*memcpy(fbp, rgb, 800*480*4);*/

    return Output_pFrame;
}

void resolutionChange(unsigned char *pointer, int row, int cols)
{
    Input_pFrame = av_frame_alloc();
    Output_pFrame = av_frame_alloc();

    img_ctx = sws_getContext(row, cols, AV_PIX_FMT_RGB32, row, cols, AV_PIX_FMT_RGB32, SWS_FAST_BILINEAR, NULL, NULL, NULL);

    avpicture_fill((AVPicture*)Output_pFrame, (uint8_t const *)fbp, AV_PIX_FMT_RGB32, 800, 480);
    avpicture_fill((AVPicture*)Input_pFrame, pointer, AV_PIX_FMT_RGB32, row, cols);

    sws_scale(img_ctx, Input_pFrame->data, Input_pFrame->linesize, 0, cols, Output_pFrame->data, Output_pFrame->linesize);
}

typedef struct {
	unsigned char cfType[2];
	unsigned int cfSize;
	unsigned int cfReserved;
	unsigned int cfoffBits;
}__attribute__((packed)) BITMAPFILEHEADER;

typedef struct
{
    unsigned int       ciSize;            //BITMAPFILEHEADER所占的字节数
    unsigned int       ciWidth;           //宽度
    unsigned int       ciHeight;          //高度
    unsigned short int ciPlanes;          //目标设备的位平面数，值为1
    unsigned short int ciBitCount;        //每个像素的位数
    char               ciCompress[4];     //压缩说明
    unsigned int       ciSizeImage;       //用字节表示的图像大小，该数据必须是4的倍数
    unsigned int       ciXPelsPerMeter;   //目标设备的水平像素数/米
    unsigned int       ciYPelsPerMeter;   //目标设备的垂直像素数/米
    unsigned int       ciClrUsed;         //位图使用调色板的颜色数
    unsigned           intciClrImportant; //指定重要的颜色数，当该域的值等于颜色数时（或者等于0时），表示所有颜色都一样重要
} __attribute__((packed)) BITMAPINFOHEADER;

void saveBmp(unsigned char *ch)
{
	BITMAPFILEHEADER bf;
	BITMAPINFOHEADER bi;
	FILE *fp1 = fopen("/root/picture/1.bmp", "wb");
	
	//Set BITMAPINFOHEADER
    memset(&bi, 0, sizeof(BITMAPINFOHEADER));
    bi.ciSize      = 40;
    bi.ciWidth     = 800;
    bi.ciHeight    = 480;
    bi.ciPlanes    = 1;
    bi.ciBitCount  = 24;
    bi.ciSizeImage = 800 * 480 * 4;


    //Set BITMAPFILEHEADER
    memset(&bf, 0, sizeof(BITMAPFILEHEADER));
    bf.cfType[0]  = 'B';
    bf.cfType[1]  = 'M';
    bf.cfSize     = 54 + bi.ciSizeImage;
    bf.cfReserved = 0;
    bf.cfoffBits  = 54;


    fwrite(&bf, 14, 1, fp1);
    fwrite(&bi, 40, 1, fp1);

	fwrite(ch, bi.ciSizeImage, 1, fp1);
	printf("save bmp ok \r\n");
}

void v4l2_close(void)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (fp < 0) {
        printf("close failed\r\n");
        exit(-1);
    }
    
    if (ioctl(fp, VIDIOC_STREAMOFF, &type) == -1) {
        printf("close stream failed\r\n");
        exit(-1);
    }

    for (i = 0;i < 4;i++) {
        if (!munmap(buffers[i].start, buffers[i].length)) {
            printf("error to munmap\r\n");
            exit(-1);
        }
    }

    /*if(rgb)free(rgb);*/
    if(Input_pFrame)av_free(Input_pFrame);
    if(Output_pFrame)av_free(Output_pFrame);
    if(img_ctx)sws_freeContext(img_ctx);

    free(buffers);
    close(fp);
    printf("v4l2 close\r\n");
}
