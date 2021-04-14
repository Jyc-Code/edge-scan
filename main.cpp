#include <stdio.h>
#include "lcd.h"
#include "ffmpeg.h"
#include "opencv.hpp"

EDGE_TYPE gEdge_Type = UNKNOW;

int main(int argc, char *argv[])
{
    sBUFFER *sYuv = NULL;

	v4l2_init();
	lcd_init();
	
	while (1) {
		sYuv = v4l2_get();
        opencvEdge(gEdge_Type, (unsigned char *)sYuv);
	}
	
	v4l2_close();
	lcd_close();
	return 0;
}
