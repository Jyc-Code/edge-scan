#ifndef _OPENCV_HPP
#define _OPENCV_HPP

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/mat.hpp>

#ifdef __cplusplus
extern "C" {
#endif

#include "print.h"
#include "lcd.h"
#include "ffmpeg.h"

#ifdef __cplusplus
}
#endif

typedef enum {
    UNKNOW = 0,
    CANNY = 1,
    SOBLE = 2,
    LAPLACIAN = 3,
}EDGE_TYPE;

cv::Mat sYUYV2BGR32(uint8_t *pYuyvData);
void opencvEdge(EDGE_TYPE sEdgeType, uint8_t *pYuyvData);

#endif

