#include "../internal.h"
#include "x11_internal.h"
#include <mr_utils.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// @TODO err handling
void
window_init(clk_Window **window, int window_x, int window_y, int window_w,
	    int window_h, int border_w)
{
	struct x11_Window *const x11_window = malloc(sizeof(*x11_window));

	x11_window->main_display = XOpenDisplay(0);

	x11_window->root_window = XDefaultRootWindow(x11_window->main_display);

	x11_window->wm_delete_window = XInternAtom(x11_window->main_display,
						   "WM_DELETE_WINDOW", False);

	x11_window->context = XDefaultGC(x11_window->main_display, 0);

	XSetForeground(x11_window->main_display, x11_window->context,
		       WhitePixel(x11_window->main_display,
				  DefaultScreen(x11_window->main_display)));

	int AttributeValueMask = CWBackPixel | CWEventMask;
	XSetWindowAttributes WindowAttributes = { 0 };

	WindowAttributes.event_mask = StructureNotifyMask | ExposureMask |
				      KeyPressMask | KeyReleaseMask |
				      ButtonPressMask | ButtonReleaseMask |
				      PointerMotionMask;

	WindowAttributes.background_pixel = WINDOW_BACKGROUND_COLOR;

	x11_window->main_window =
		XCreateWindow(x11_window->main_display, x11_window->root_window,
			      window_x, window_y, window_w, window_h, border_w,
			      CopyFromParent, CopyFromParent, CopyFromParent,
			      AttributeValueMask, &WindowAttributes);

	XStoreName(x11_window->main_display, x11_window->main_window,
		   PROGRAM_NAME);

	XSetWMProtocols(x11_window->main_display, x11_window->main_window,
			&x11_window->wm_delete_window, 1);

	XMapWindow(x11_window->main_display, x11_window->main_window);

	*window = x11_window;
}

int
window_free(clk_Window *window)
{
	struct x11_Window *const x11_window = window;

	int err = XDestroyWindow(x11_window->main_display,
				 x11_window->main_window);

	XCloseDisplay(x11_window->main_display);
	free(x11_window);

	return err;
}

void
window_pol_event(void)
{
	struct x11_Window *const x11_window = clicker_renderer.window_ctx;

	XEvent GeneralEvent = { 0 };
	XNextEvent(x11_window->main_display, &GeneralEvent);

	memset(&clicker_event, 0, sizeof(clicker_event));

	if (GeneralEvent.xany.window != x11_window->main_window)
		return;

	switch (GeneralEvent.type) {
	case KeyPress:
		clicker_event.type = CLK_WINDOW_EVENT_TYPE_KEYDOWN;
		clicker_event.val.keycode = GeneralEvent.xkey.keycode;
		break;

	case KeyRelease:
		clicker_event.type = CLK_WINDOW_EVENT_TYPE_KEYUP;
		clicker_event.val.keycode = GeneralEvent.xkey.keycode;
		break;

	case ButtonPress:
		clicker_event.type = CLK_WINDOW_EVENT_TYPE_MOUSEDOWN;
		clicker_event.val.mouse.x = GeneralEvent.xbutton.x;
		clicker_event.val.mouse.y = GeneralEvent.xbutton.y;
		clicker_event.val.mouse.button = GeneralEvent.xbutton.button;
		break;

	case ButtonRelease:
		clicker_event.type = CLK_WINDOW_EVENT_TYPE_MOUSEUP;
		clicker_event.val.mouse.x = GeneralEvent.xbutton.x;
		clicker_event.val.mouse.y = GeneralEvent.xbutton.y;
		clicker_event.val.mouse.button = GeneralEvent.xbutton.button;
		break;

	case MotionNotify:
		clicker_event.type = CLK_WINDOW_EVENT_TYPE_MOUSEMOVE;
		clicker_event.val.mouse.x = GeneralEvent.xmotion.x;
		clicker_event.val.mouse.y = GeneralEvent.xmotion.y;
		clicker_event.val.mouse.button = 0;
		break;

	case ClientMessage: {
		if ((Atom)GeneralEvent.xclient.data.l[0] ==
		    x11_window->wm_delete_window) {
			clicker_event.type = CLK_WINDOW_EVENT_TYPE_CLOSEREQ;
		}
		break;
	}

		// use this for window resize updates
	case ConfigureNotify: {
		text_update_text_surface_to_window_size(
			&clicker_renderer.text_ctx,
			clicker_renderer.window_ctx);
		break;
	}

	case EnterNotify:
	case LeaveNotify:
	default:
		break;
	}
}

void
window_clear(clk_Window *const window)
{
	struct x11_Window *const x11_window = window;
	XClearWindow(x11_window->main_display, x11_window->main_window);
}

void
window_flush_display(clk_Window *window)
{
	struct x11_Window *const x11_window = window;
	XFlush(x11_window->main_display);
}

void
window_draw_fill_rectangle(clk_Window *const window, uint16_t x, uint16_t y,
			   uint16_t w, uint16_t h)
{
	struct x11_Window *const x11_window = window;
	XFillRectangle(x11_window->main_display, x11_window->main_window,
		       x11_window->context, x, y, w, h);
}
