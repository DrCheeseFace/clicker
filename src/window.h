#ifndef WINDOW_H
#define WINDOW_H

#include <stdint.h>

typedef void clk_Window;

enum clk_WindowEventType {
	CLK_WINDOW_EVENT_TYPE_NONE,
	CLK_WINDOW_EVENT_TYPE_KEYDOWN,
	CLK_WINDOW_EVENT_TYPE_KEYUP,
	CLK_WINDOW_EVENT_TYPE_MOUSEDOWN,
	CLK_WINDOW_EVENT_TYPE_MOUSEUP,
	CLK_WINDOW_EVENT_TYPE_MOUSEMOVE,
	CLK_WINDOW_EVENT_TYPE_CLOSEREQ,
};

enum clk_WindowEventMouse {
	CLK_WINDOW_EVENT_MOUSE1,
	CLK_WINDOW_EVENT_MOUSE2,
	CLK_WINDOW_EVENT_MOUSE3,
	CLK_WINDOW_EVENT_MOUSE_SCROLL_UP,
	CLK_WINDOW_EVENT_MOUSE_SCROLL_DOWN,

};

struct clk_WindowEvent {
	enum clk_WindowEventType type;

	union {
		struct {
			enum clk_WindowEventMouse button;
			uint16_t x;
			uint16_t y;
		} mouse;

		uint16_t keycode;

	} val;
};

clk_Window *window_create(int window_x, int window_y, int window_w,
			  int window_h, int border_w);
int window_destroy(clk_Window *window);

void window_get_event(clk_Window *window, struct clk_WindowEvent *event);

void window_clear(clk_Window *window);
void window_flush_display(clk_Window *window);

#ifdef DEBUG
void window_draw_debug_snack(clk_Window *window, const char *text);
#endif

#endif
