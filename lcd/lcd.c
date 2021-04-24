#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>

#include "lcd.h"

int frame_x = 0;
static int fb = 0;
static long screensize = 0;
void *fbp = NULL;

void lcd_init(void)
{
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;
 
	fb = open("/dev/fb0", O_RDWR);
    if (fb < 0) {
        printf("Can't open framebuffer device!\r\n");
        exit(1);
    }

    if (ioctl(fb, FBIOGET_FSCREENINFO, &finfo)) {
        printf("Can't read fix information!\r\n");
    }

    if (ioctl(fb, FBIOGET_VSCREENINFO, &vinfo)) {
        printf("Can't read var information!\r\n");
    }
	/* 153600 */
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    frame_x = vinfo.xres;
	/* 进行地址映射 */
    fbp = mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);
	
	if (fbp == (void *)(-1)) {
		printf("error memory map!\r\n");
		close(fb);
		exit(-1);
	}
}

void lcd_close(void)
{
	munmap(fbp, screensize);
	close(fb);
}
/*
void lcd_write(void *img_buf, unsigned int img_x, unsigned int img_y, unsigned int img_bits)
{
	int row, column;
	int num = 0;

	frame_rgb24 *rgb24_fbp = (frame_rgb24 *)fbp;
	rgb24 *rgb24_img_buf = (rgb24 *)img_buf;
}
*/

