#include <stdio.h>
#include "lcd.h"
#include "ffmpeg.h"
#include "opencv.hpp"

int main(int argc, char *argv[])
{
	struct buffer *yuv = NULL;

	v4l2_init();
	lcd_init();
	
	while (1) {
		yuv = v4l2_get();
		CannyEdgeByYUVV((unsigned char *)yuv->start);
	}
	
	v4l2_close();
	lcd_close();
	return 0;
}
