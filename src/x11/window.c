#include "../internal.h"
#include "x11_internal.h"
#include <mr_utils.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// @TODO err handling
void
window_init(struct clk_Window *window, int window_x, int window_y, int window_w,
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

	(*window).window_w = window_w;
	(*window).window_h = window_h;
	(*window).window_ctx = x11_window;

	// utf8 boooolshit
	x11_window->xim = XOpenIM(x11_window->main_display, NULL, NULL, NULL);
	x11_window->xic =
		XCreateIC(x11_window->xim, XNInputStyle,
			  XIMPreeditNothing | XIMStatusNothing, XNClientWindow,
			  x11_window->main_window, XNFocusWindow,
			  x11_window->main_window, NULL);
}

int
window_free(struct clk_Window window)
{
	struct x11_Window *const x11_window = window.window_ctx;

	int err = XDestroyWindow(x11_window->main_display,
				 x11_window->main_window);

	XDestroyIC(x11_window->xic);
	XCloseIM(x11_window->xim);

	XCloseDisplay(x11_window->main_display);
	free(x11_window);

	return err;
}

void
window_update_window_size(struct clk_Window *window)
{
	struct x11_Window *const x11_window = window->window_ctx;

	XWindowAttributes attributes;
	XGetWindowAttributes(x11_window->main_display, x11_window->main_window,
			     &attributes);

	window->window_w = attributes.width;
	window->window_h = attributes.height;
}

void
window_pol_event(void)
{
	struct x11_Window *const x11_window =
		clicker_renderer.clk_window.window_ctx;

	XEvent GeneralEvent = { 0 };
	XNextEvent(x11_window->main_display, &GeneralEvent);

	/* xim support need to filter */
	if (XFilterEvent(&GeneralEvent, x11_window->main_window))
		return;

	memset(&clicker_event, 0, sizeof(clicker_event));

	if (GeneralEvent.xany.window != x11_window->main_window)
		return;

	switch (GeneralEvent.type) {
	case KeyPress: {
		clicker_event.type = CLK_WINDOW_EVENT_TYPE_KEYDOWN;
		clicker_event.val.key.keycode = GeneralEvent.xkey.keycode;

		KeySym keysym = NoSymbol;

		int count = Xutf8LookupString(
			x11_window->xic, &GeneralEvent.xkey,
			clicker_event.val.key.utf8,
			sizeof(clicker_event.val.key.utf8) - 1, &keysym, NULL);

		if (count < 0 ||
		    count >= (int)sizeof(clicker_event.val.key.utf8)) {
			count = 0;
		}

		clicker_event.val.key.utf8[count] = '\0';

		break;
	}

	case KeyRelease: {
		clicker_event.type = CLK_WINDOW_EVENT_TYPE_KEYUP;
		clicker_event.val.key.keycode = GeneralEvent.xkey.keycode;
		clicker_event.val.key.utf8[0] = '\0';
		break;
	}

	case ButtonPress: {
		clicker_event.type = CLK_WINDOW_EVENT_TYPE_MOUSEDOWN;
		clicker_event.val.mouse.x = GeneralEvent.xbutton.x;
		clicker_event.val.mouse.y = GeneralEvent.xbutton.y;
		clicker_event.val.mouse.button = GeneralEvent.xbutton.button;
		break;
	}

	case ButtonRelease: {
		clicker_event.type = CLK_WINDOW_EVENT_TYPE_MOUSEUP;
		clicker_event.val.mouse.x = GeneralEvent.xbutton.x;
		clicker_event.val.mouse.y = GeneralEvent.xbutton.y;
		clicker_event.val.mouse.button = GeneralEvent.xbutton.button;
		break;
	}

	case MotionNotify: {
		clicker_event.type = CLK_WINDOW_EVENT_TYPE_MOUSEMOVE;
		clicker_event.val.mouse.x = GeneralEvent.xmotion.x;
		clicker_event.val.mouse.y = GeneralEvent.xmotion.y;
		clicker_event.val.mouse.button = 0;
		break;
	}

	case ClientMessage: {
		if ((Atom)GeneralEvent.xclient.data.l[0] ==
		    x11_window->wm_delete_window) {
			clicker_event.type = CLK_WINDOW_EVENT_TYPE_CLOSEREQ;
		}
		break;
	}

	case ConfigureNotify: {
		clicker_event.type = CLK_WINDOW_EVENT_TYPE_RESIZEREQ;
		break;
	}

	case EnterNotify:
	case LeaveNotify:
	default:
		break;
	}
}

void
window_clear(struct clk_Window window)
{
	struct x11_Window *const x11_window = window.window_ctx;
	XClearWindow(x11_window->main_display, x11_window->main_window);
}

void
window_flush_display(struct clk_Window window)
{
	struct x11_Window *const x11_window = window.window_ctx;
	XFlush(x11_window->main_display);
}

// @TODO replace with cairo
void
window_draw_fill_rectangle(struct clk_Window window, uint16_t x, uint16_t y,
			   uint16_t w, uint16_t h)
{
	struct x11_Window *const x11_window = window.window_ctx;
	XFillRectangle(x11_window->main_display, x11_window->main_window,
		       x11_window->context, x, y, w, h);
}

// @TODO replace with cairo
void
window_draw_line(struct clk_Window window, uint16_t x1, uint16_t y1,
		 uint16_t x2, uint16_t y2)
{
	struct x11_Window *const x11_window = window.window_ctx;

	XDrawLine(x11_window->main_display, x11_window->main_window,
		  x11_window->context, x1, y1, x2, y2);
}
