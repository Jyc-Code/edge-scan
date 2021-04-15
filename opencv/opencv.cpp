#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <pthread.h>
#include <unistd.h>

#include "lcd.h"
#include "opencv.hpp"

extern pthread_mutex_t gGetYuyvMutex;
static cv::Mat sYUYV2BGR32(unsigned char *pYuyvData);

using namespace cv;
/**
 * Mat类的data属性是一个uchar的指针
 * */

static cv::Mat sYUYV2BGR32(unsigned char *pYuyvData)
{
    /* opencv Mat (cols, rows) */
    Mat yuvImg(480, 640, CV_8UC2);
    Mat srcRgbImg(480, 640, CV_8UC4);
    Mat dstImg(480, 640, CV_8UC4);

    memcpy(yuvImg.data, pYuyvData, 640*480*2);
    cvtColor(yuvImg, srcRgbImg, CV_YUV2BGRA_YUYV);
    
    //转成多通道BGR
    unsigned char *ch = srcRgbImg.data;
    
    for (int j = 0; j < 480; j++) {
        for (int i = 0;i < 640; i++) {
            // if (i > 640 && *(ch))
            dstImg.at<Vec4b>(j,i)[0] = (unsigned char)(*(ch));
            dstImg.at<Vec4b>(j,i)[1] = (unsigned char)(*(ch + 1));
            dstImg.at<Vec4b>(j,i)[2] = (unsigned char)(*(ch + 2));
            dstImg.at<Vec4b>(j,i)[3] = (unsigned char)(*(ch + 3));

            ch += 4;
        }
    }

    return dstImg;
}

void opencvEdge(EDGE_TYPE sEdgeType, unsigned char * pYuyvData)
{   
    Mat rgbImg(480, 640,CV_8UC4);
    Mat dst, edge, gray;
    sFRAME_RGB32 *temp = (sFRAME_RGB32 *)malloc(rgbImg.rows * rgbImg.cols);
    
    pthread_mutex_lock(&gGetYuyvMutex);
    rgbImg = sYUYV2BGR32_Opencv(pYuyvData);
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
    for (int i = 0,j = 0;i < 640*480;i++) 
    {
        j = i/640;
        temp[i].b = (unsigned char)dst.at<Vec4b>(j,i%640)[0];
        temp[i].g = (unsigned char)dst.at<Vec4b>(j,i%640)[1];
        temp[i].r = (unsigned char)dst.at<Vec4b>(j,i%640)[2];
        temp[i].a = (unsigned char)dst.at<Vec4b>(j,i%640)[3];
        // temp[i].a = 0;
    }

    resolutionChange((unsigned char *)temp, 640, 480);
    free(temp);
}
