#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <pthread.h>
#include <unistd.h>

#include "lcd.h"
#include "opencv.hpp"
#include "opencv2/core/matx.hpp"

extern pthread_mutex_t gGetYuyvMutex;
static cv::Mat sYUYV2BGR32(uint8_t *pYuyvData);

using namespace cv;

static cv::Mat sYUYV2BGR32(uint8_t *pYuyvData)
{
    /* opencv Mat (cols, rows) */
    Mat dstImg(480, 640, CV_8UC4);
    
    dstImg.data = yuyv2rgb24_ffmpeg(pYuyvData);
#if 0
    //转成多通道BGR
    uint8_t *ch = yuyv2rgb24_ffmpeg(pYuyvData);
    Mat_<Vec4b>::iterator it = dstImg.begin<Vec4b>();
#if 0
    for (int i = 0; i < 480*640;i++) 
#else
    for (;it != dstImg.end<Vec4b>();it++)
#endif
    {
        /*
         * dstImg.at<Vec4b>(i/640, i%640)[0] = (uint8_t)(*(ch));
         * dstImg.at<Vec4b>(i/640, i%640)[1] = (uint8_t)(*(ch + 1));
         * dstImg.at<Vec4b>(i/640, i%640)[2] = (uint8_t)(*(ch + 2));
         * dstImg.at<Vec4b>(i/640, i%640)[3] = (uint8_t)(*(ch + 3));
         */
        (*it)[0] = (uint8_t)(*(ch));
        (*it)[1] = (uint8_t)(*(ch + 1));
        (*it)[2] = (uint8_t)(*(ch + 2));
        (*it)[3] = (uint8_t)(*(ch + 3));
        ch += 4;
    }
#endif
    return dstImg;
}

void opencvEdge(EDGE_TYPE sEdgeType, uint8_t * pYuyvData)
{   
    Mat rgbImg(480, 640,CV_8UC4);
    Mat dst, edge, gray;
    sFRAME_RGB32 *temp = (sFRAME_RGB32 *)malloc(rgbImg.rows * rgbImg.cols);
    
    pthread_mutex_lock(&gGetYuyvMutex);
    rgbImg = sYUYV2BGR32(pYuyvData);
    pthread_mutex_unlock(&gGetYuyvMutex);

    cvtColor(rgbImg, gray, COLOR_BGRA2GRAY);
    dst.create(rgbImg.size(), rgbImg.type());
    dst = Scalar::all(0);

    switch(sEdgeType) 
    {
        case UNKNOW :
                    break;
        case CANNY :
                    blur(gray, edge, Size(3, 3));
                    Canny(edge, edge, 3, 9, 3);
                    rgbImg.copyTo(dst, edge);
                    break;
        case SOBLE :
                    break;
        case LAPLACIAN :
                    // GaussianBlur()
                    Laplacian(gray, dst, CV_16S, 3, 1, 0, BORDER_DEFAULT);
                    convertScaleAbs(dst, edge);
                    break;

        default:break;
    }
    //重新转换成单通道RGBA的格式
    for (int i = 0;i < 640*480;i++) 
    {
        temp[i].b = (uint8_t)dst.at<Vec4b>(i/640,i%640)[0];
        temp[i].g = (uint8_t)dst.at<Vec4b>(i/640,i%640)[1];
        temp[i].r = (uint8_t)dst.at<Vec4b>(i/640,i%640)[2];
        temp[i].a = (uint8_t)dst.at<Vec4b>(i/640,i%640)[3];
        // temp[i].a = 0;
    }

    resolutionChange((uint8_t *)temp, 640, 480);
    free(temp);
}
