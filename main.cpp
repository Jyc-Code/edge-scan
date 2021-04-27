#include <cstddef>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>

#include "lcd.h"
#include "ffmpeg.h"
#include "opencv.hpp"
#include "print.h"

using namespace cv;

pthread_t gKeyPid;
pthread_t gGetYuyvPid;
pthread_t gOpencvPid;
pthread_mutex_t gGetYuyvMutex = PTHREAD_MUTEX_INITIALIZER;

EDGE_TYPE gEdge_Type = UNKNOW;
sBUFFER *gsYuv = NULL;

//key
uint8_t gSwitch = 0;
uint8_t gChangeFlag = 0;
const char *filename = "/dev/imx6uirq_key";
int gKeyFb;
#define KEYVALUE 0XF0

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

void *pthread_keyTask(void *ptr)
{
    int ret = 0;
    uint8_t KeyValue;
    while(1)
    {
        ret = read(gKeyFb, &KeyValue, sizeof(KeyValue));
        if(ret < 0 || KeyValue != 1)
        {
            usleep(50);
        }
        else
        {
            gSwitch++;
        }
    }
}

void *pthread_GetYuyvTask(void *ptr)
{
    while(1)
    {
        gsYuv = v4l2_get();
        usleep(1);
    }
    return 0;
}

void *pthread_OpencvTask(void *ptr)
{
    while(1)
    {
        if(gsYuv != NULL)
        {
            switch (gSwitch) 
            {
                case 0 :
                        gEdge_Type = UNKNOW;
                        break;
                case 1 : 
                        gEdge_Type = CANNY;
                    break;
                case 2 : 
                        gEdge_Type = SOBLE;
                        break;
                case 3 :
                        gEdge_Type = LAPLACIAN;
                        break;
                case 4 : 
                        gChangeFlag = 1;
                        gSwitch = 0;
                        break;
             
                default:break;
            }
            opencvEdge(gEdge_Type, (uint8_t *)gsYuv->start);
        }
        usleep(5);
    }

    return 0;
}

void dataInit(void)
{
    Input_pFrame = av_frame_alloc();
    Output_pFrame = av_frame_alloc();

    rgb = (uint8_t *)malloc(640*480*3);
    // bgrImg = Scalar::all(0);

    gKeyFb = open(filename, O_RDWR);
    if(gKeyFb < 0)
    {
        ERR("File %s open failed!\n", filename);
    }
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
    gKeyPid = 2;
    if(pthread_create(&gKeyPid, NULL, pthread_keyTask, NULL) != 0)
    {
        ERR("pthread_create pthread_keyTask failed! \n");
    }

    pthread_join(gGetYuyvPid, NULL);
    pthread_join(gOpencvPid, NULL);

    close(gKeyFb);
	v4l2_close();
	lcd_close();
	return 0;
}
