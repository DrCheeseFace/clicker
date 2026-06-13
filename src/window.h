#ifndef WINDOW_H
#define WINDOW_H

#include <stdint.h>
#define WINDOW_BACKGROUND_COLOR 0x00022424;
typedef void clicker_Window;

enum clicker_WindowEventType {
	CLICKER_WINDOW_EVENT_TYPE_NONE,
	CLICKER_WINDOW_EVENT_TYPE_KEYDOWN,
	CLICKER_WINDOW_EVENT_TYPE_KEYUP,
	CLICKER_WINDOW_EVENT_TYPE_MOUSEDOWN,
	CLICKER_WINDOW_EVENT_TYPE_MOUSEUP,
	CLICKER_WINDOW_EVENT_TYPE_MOUSEMOVE,
	CLICKER_WINDOW_EVENT_TYPE_CLOSEREQ,
};

enum clicker_WindowEventButton {
	CLICKER_WINDOW_EVENT_BUTTON1,
	CLICKER_WINDOW_EVENT_BUTTON2,
	CLICKER_WINDOW_EVENT_BUTTON3,
};

struct clicker_WindowEvent {
	enum clicker_WindowEventType type;

	union {
		struct {
			enum clicker_WindowEventButton button;
			uint16_t x;
			uint16_t y;
		} mouse;

		uint16_t keycode;

	} val;
};

void window_init(void);
void window_cleanup(void);

clicker_Window *window_create(int window_x, int window_y, int window_w,
			      int window_h, int border_w);

int window_destroy(clicker_Window *window);

void window_get_event(clicker_Window *window,
		      struct clicker_WindowEvent *event);

void window_clear(clicker_Window *window);
void window_flush_display(void);

#ifdef DEBUG
void window_draw_debug_snack(clicker_Window *window, const char *text);
#endif

#endif
