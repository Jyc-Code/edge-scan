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
    return dstImg;
}

void opencvEdge(EDGE_TYPE sEdgeType, uint8_t *pYuyvData)
{   
    Mat dst, edge, gray, grad_x, grad_y, bgrImg;

    /*
     * dst.create(bgrImg.size(), bgrImg.type());
     * dst = Scalar::all(0);
     */

    switch(sEdgeType) 
    {
        case UNKNOW :
                    resolutionChange(pYuyvData, 640, 480);
                    break;
        case CANNY :
                    bgrImg.data = yuyv2rgb24_ffmpeg(pYuyvData);
                    cvtColor(bgrImg, gray, COLOR_BGR2GRAY);
                    blur(gray, edge, Size(3, 3));
                    Canny(edge, edge, 3, 9, 3);
                    bgrImg.copyTo(dst, edge);
                    resolutionChange(dst.data, 640, 480);
                    break;
        case SOBLE :
                    bgrImg.data = yuyv2rgb24_ffmpeg(pYuyvData);
                    GaussianBlur(bgrImg, bgrImg, Size(3, 3), 0);
                    cvtColor(bgrImg, gray, COLOR_BGR2GRAY);
                    Sobel(gray, grad_x, CV_16S, 1, 0, 3, 1, 1,BORDER_DEFAULT);
                    convertScaleAbs(grad_x, grad_x);
                    Sobel(gray, grad_y, CV_16S, 0, 1, 3, 1, 1,BORDER_DEFAULT);
                    convertScaleAbs(grad_y, grad_y);
                    addWeighted(grad_x, 0.5, grad_y, 0.5, 0, dst);
                    resolutionChange(dst.data, 640, 480);
                    break;
        case LAPLACIAN :
                    bgrImg.data = yuyv2rgb24_ffmpeg(pYuyvData);
                    cvtColor(bgrImg, gray, COLOR_BGR2GRAY);
                    Laplacian(gray, edge, CV_16S, 3, 1, 0, BORDER_DEFAULT);
                    convertScaleAbs(edge, dst);
                    resolutionChange(dst.data, 640, 480);
                    break;

        default:break;
    }
}
