#ifndef _OPENCV_HPP
#define _OPENCV_HPP

typedef enum {
    UNKNOW = 0,
    CANNY = 1,
    SOBLE = 2,
    LAPLACIAN = 3,
}EDGE_TYPE;

#ifdef __cplusplus
extern "C" {
#endif

#include "lcd.h"
#include "ffmpeg.h"

#ifdef __cplusplus
}
#endif

void opencvEdge(EDGE_TYPE sEdgeType, unsigned char * pYuyvData);

#endif

