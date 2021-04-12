#ifndef _LCD_H
#define _LCD_H

typedef struct {
    unsigned char b;
    unsigned char g;
    unsigned char r;
	unsigned char a;
}rgb32, frame_rgb32;

#ifdef __cplusplus
extern "C" {
#endif

void lcd_init(void);
void lcd_close(void);

#ifdef __cplusplus
}
#endif

#endif 

