#ifndef WINDOW_H
#define WINDOW_H

#define WINDOW_BACKGROUND_COLOR 0x00022424;
typedef void clicker_Window;

void window_init(void);

clicker_Window *window_create(int window_x, int window_y, int window_w,
			      int window_h, int border_w);

int window_destroy(clicker_Window *window);

#endif
