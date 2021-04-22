#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include "ffmpeg.h"
#include "lcd.h"
#include "opencv.hpp"
#include "opencv2/imgproc.hpp"

using namespace cv;

cv::Mat sYUYV2BGR32(uint8_t *pYuyvData)
{
    /* opencv Mat (cols, rows) */
    Mat dstImg(480, 640, CV_8UC3);
    
    dstImg.data = yuyv2rgb24_ffmpeg(pYuyvData);
    // memcpy(dstImg.data, yuyv2rgb24_ffmpeg(pYuyvData), 640*480*3);
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

void opencvEdge(EDGE_TYPE sEdgeType, cv::Mat bgrImg)
{   
    Mat dst, edge, gray, grad_x, grad_y;
    // sFRAME_RGB32 *temp = (sFRAME_RGB32 *)malloc(bgrImg.rows * bgrImg.cols * 3);

    dst.create(bgrImg.size(), bgrImg.type());
    dst = Scalar::all(0);

    switch(sEdgeType) 
    {
        case UNKNOW :
                    resolutionChange(bgrImg.data, 640, 480);
                    break;
        case CANNY :
                    cvtColor(bgrImg, gray, COLOR_BGR2GRAY);
                    blur(gray, edge, Size(3, 3));
                    Canny(edge, edge, 3, 9, 3);
                    bgrImg.copyTo(dst, edge);
                    resolutionChange(dst.data, 640, 480);
                    break;
        case SOBLE :
                    GaussianBlur(bgrImg, bgrImg, Size(3, 3), 0);
                    cvtColor(bgrImg, gray, COLOR_BGR2GRAY);
                    Sobel(gray, grad_x, CV_16S, 1, 0, 3, 1, 1,BORDER_DEFAULT);
                    convertScaleAbs(grad_x, grad_x);
                    Sobel(gray, grad_y, CV_16S, 0, 1, 3, 1, 1,BORDER_DEFAULT);
                    convertScaleAbs(grad_y, grad_y);
                    addWeighted(grad_x, 0.5, grad_y, 0.5, 0, dst);
                    cvtColor(dst, dst, COLOR_GRAY2BGR);
                    resolutionChange(dst.data, 640, 480);
                    break;
        case LAPLACIAN :
                    // GaussianBlur()
                    Laplacian(gray, edge, CV_16S, 3, 1, 0, BORDER_DEFAULT);
                    convertScaleAbs(edge, dst);
                    cvtColor(dst, dst, COLOR_GRAY2BGR);
                    resolutionChange(dst.data, 640, 480);
                    break;

        default:break;
    }
    //重新转换成单通道RGBA的格式
    /*
     * for (int i = 0;i < 640*480;i++) 
     * {
     *     temp[i].b = (uint8_t)dst.at<Vec4b>(i/640,i%640)[0];
     *     temp[i].g = (uint8_t)dst.at<Vec4b>(i/640,i%640)[1];
     *     temp[i].r = (uint8_t)dst.at<Vec4b>(i/640,i%640)[2];
     *     temp[i].a = (uint8_t)dst.at<Vec4b>(i/640,i%640)[3];
     *     // temp[i].a = 0;
     * }
     */

    // resolutionChange((uint8_t *)temp, 640, 480);
    // free(temp);
}
