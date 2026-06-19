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

enum clk_WindowEventButton {
	CLK_WINDOW_EVENT_BUTTON1,
	CLK_WINDOW_EVENT_BUTTON2,
	CLK_WINDOW_EVENT_BUTTON3,
};

struct clk_WindowEvent {
	enum clk_WindowEventType type;

	union {
		struct {
			enum clk_WindowEventButton button;
			uint16_t x;
			uint16_t y;
		} mouse;

		uint16_t keycode;

	} val;
};

void window_init(void);
void window_cleanup(void);

clk_Window *window_create(int window_x, int window_y, int window_w,
			  int window_h, int border_w);
int window_destroy(clk_Window *window);

void window_get_event(clk_Window *window, struct clk_WindowEvent *event);

void window_clear(clk_Window *window);
void window_flush_display(void);

#ifdef DEBUG
void window_draw_debug_snack(clk_Window *window, const char *text);
#endif

#endif
