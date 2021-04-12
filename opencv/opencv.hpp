#ifndef _OPENCV_HPP
#define _OPENCV_HPP

#ifdef __cplusplus
extern "C" {
#endif

#include "lcd.h"
#include "ffmpeg.h"

#ifdef __cplusplus
}
#endif

void CannyEdge(AVFrame *iAvFrame);
void CannyEdgeByYUVV(unsigned char *input);

#endif

