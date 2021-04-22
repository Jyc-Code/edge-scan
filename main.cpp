#include <cstddef>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#include "lcd.h"
#include "ffmpeg.h"
#include "opencv.hpp"
#include "print.h"

using namespace cv;

pthread_t gGetYuyvPid;
pthread_t gOpencvPid;
pthread_mutex_t gGetYuyvMutex = PTHREAD_MUTEX_INITIALIZER;

EDGE_TYPE gEdge_Type = UNKNOW;
sBUFFER *gsYuv = NULL;

Mat bgrImg(480, 640, CV_8UC3);
uint8_t *rgb;
AVFrame *Input_pFrame;
AVFrame *Output_pFrame;
extern void showT(uint8_t u8mode, const char *ch);
/*
 *  用于获取代码段执行时间
 *  mode 0 开始 1 结束
 */
void showT(uint8_t u8mode, const char *ch)
{
    static struct timeval sFirst_time;
    static struct timeval sLast_time;
    struct timeval Second_time;
    switch(u8mode)
    {
        case 0 : 
                gettimeofday(&sFirst_time, NULL);
                sLast_time = sFirst_time;
                break;
        case 1 :
                gettimeofday(&Second_time, NULL);
                DEBUG("~~~~~~~~~~T0 --> %s:%d us\n", ch, (int)((Second_time.tv_sec - sLast_time.tv_sec) * 1000000 + Second_time.tv_usec - sLast_time.tv_usec));
                sLast_time = Second_time;
                break;
    }
}

void *pthread_GetYuyvTask(void *ptr)
{
    while(1)
    {
        gsYuv = v4l2_get();
        bgrImg = sYUYV2BGR32((uint8_t *)gsYuv->start);
        usleep(5);
    }
    return 0;
}

void *pthread_OpencvTask(void *ptr)
{
    while(1)
    {
        if(gsYuv != NULL)
            opencvEdge(gEdge_Type, bgrImg);
        usleep(10);
    }

    return 0;
}

void dataInit(void)
{
    Input_pFrame = av_frame_alloc();
    Output_pFrame = av_frame_alloc();

    rgb = (uint8_t *)malloc(640*480*3);
    bgrImg = Scalar::all(0);
}

int main(int argc, char *argv[])
{
	v4l2_init();
	lcd_init();
	dataInit();
    
    gGetYuyvPid = 0;   
    if(pthread_create(&gGetYuyvPid, NULL, pthread_GetYuyvTask, NULL) != 0)
    {
        ERR("pthread_create pthread_GetYuyvTask failed! \n");
    }
	gOpencvPid = 1;
    if(pthread_create(&gOpencvPid, NULL, pthread_OpencvTask, NULL) != 0)
    {
        ERR("pthread_create pthread_OpencvTask failed! \n");
    }

    pthread_join(gGetYuyvPid, NULL);
    pthread_join(gOpencvPid, NULL);

	v4l2_close();
	lcd_close();
	return 0;
}
