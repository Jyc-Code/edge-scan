#ifndef _FFMPEG_H
#define _FFMPEG_H

struct buffer {
    void* start;
    unsigned int length;
};

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>
#include <libavutil/frame.h>

void v4l2_init(void);
void saveBmp(unsigned char *ch);
void v4l2_close(void);
struct buffer* v4l2_get(void);
AVFrame* yuvv_2_rgb24_ffmpeg(unsigned char *pointer);
void resolutionChange(unsigned char *pointer, int row, int cols);

#ifdef __cplusplus
}
#endif

#endif

