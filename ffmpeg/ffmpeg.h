#ifndef _FFMPEG_H
#define _FFMPEG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>

typedef struct buffer {
    void* start;
    uint8_t length;
}sBUFFER;

void v4l2_init(void);
void v4l2_close(void);
sBUFFER* v4l2_get(void);
uint8_t* yuyv2rgb24_ffmpeg(uint8_t *pointer);
void resolutionChange(uint8_t *pointer, int cols, int row);

#ifdef __cplusplus
}
#endif

#endif

