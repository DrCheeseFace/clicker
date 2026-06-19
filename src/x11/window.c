#include "../window.h"
#include "../internal.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <mr_utils.h>
#include <stdlib.h>
#include <string.h>

internal void init_mouse_event(XEvent *GeneralEvent,
			       struct clk_WindowEvent *event);

struct x11_Window {
	GC context;
	Window root_window;
	Window main_window;
	Display *main_display;
	Atom wm_delete_window;
};

internal void
window_init(struct x11_Window *window)
{
	window->main_display = XOpenDisplay(0);

	window->root_window = XDefaultRootWindow(window->main_display);

	window->wm_delete_window =
		XInternAtom(window->main_display, "WM_DELETE_WINDOW", False);

	window->context = XDefaultGC(window->main_display, 0);

	XSetForeground(window->main_display, window->context,
		       WhitePixel(window->main_display,
				  DefaultScreen(window->main_display)));
}

clk_Window *
window_create(int window_x, int window_y, int window_w, int window_h,
	      int border_w)
{
	struct x11_Window *clicker_window = malloc(sizeof(*clicker_window));
	window_init(clicker_window);

	int AttributeValueMask = CWBackPixel | CWEventMask;
	XSetWindowAttributes WindowAttributes = { 0 };

	WindowAttributes.event_mask = StructureNotifyMask | ExposureMask |
				      KeyPressMask | KeyReleaseMask |
				      ButtonPressMask | ButtonReleaseMask |
				      PointerMotionMask;

	WindowAttributes.background_pixel = WINDOW_BACKGROUND_COLOR;

	clicker_window->main_window =
		XCreateWindow(clicker_window->main_display,
			      clicker_window->root_window, window_x, window_y,
			      window_w, window_h, border_w, CopyFromParent,
			      CopyFromParent, CopyFromParent,
			      AttributeValueMask, &WindowAttributes);

	XStoreName(clicker_window->main_display, clicker_window->main_window,
		   PROGRAM_NAME);

	XSetWMProtocols(clicker_window->main_display,
			clicker_window->main_window,
			&clicker_window->wm_delete_window, 1);

	XMapWindow(clicker_window->main_display, clicker_window->main_window);

	return clicker_window;
}

int
window_destroy(clk_Window *clicker_window)
{
	struct x11_Window *x11_window = clicker_window;

	int err = XDestroyWindow(x11_window->main_display,
				 x11_window->main_window);

	XCloseDisplay(x11_window->main_display);
	free(x11_window);

	return err;
}

void
window_get_event(clk_Window *clicker_window, struct clk_WindowEvent *event)
{
	struct x11_Window *x11_window = clicker_window;

	XEvent GeneralEvent = { 0 };
	XNextEvent(x11_window->main_display, &GeneralEvent);

	memset(event, 0, sizeof(*event));

	if (GeneralEvent.xany.window != x11_window->main_window)
		return;

	switch (GeneralEvent.type) {
	case KeyPress:
		event->type = CLK_WINDOW_EVENT_TYPE_KEYDOWN;
		event->val.keycode = GeneralEvent.xkey.keycode;
		break;

	case KeyRelease:
		event->type = CLK_WINDOW_EVENT_TYPE_KEYUP;
		event->val.keycode = GeneralEvent.xkey.keycode;
		break;

	case ButtonPress:
		event->type = CLK_WINDOW_EVENT_TYPE_MOUSEDOWN;
		init_mouse_event(&GeneralEvent, event);
		break;

	case ButtonRelease:
		event->type = CLK_WINDOW_EVENT_TYPE_MOUSEUP;
		init_mouse_event(&GeneralEvent, event);
		break;

	case MotionNotify:
		event->type = CLK_WINDOW_EVENT_TYPE_MOUSEMOVE;
		init_mouse_event(&GeneralEvent, event);
		break;

	case ClientMessage: {
		if ((Atom)GeneralEvent.xclient.data.l[0] ==
		    x11_window->wm_delete_window) {
			event->type = CLK_WINDOW_EVENT_TYPE_CLOSEREQ;
		}
		break;
	}

	case EnterNotify:
	case LeaveNotify:
	case ResizeRequest:
	default:
		break;
	}
}

void
window_clear(clk_Window *clicker_window)
{
	struct x11_Window *x11_window = clicker_window;
	XClearWindow(x11_window->main_display, x11_window->main_window);
}

void
window_flush_display(clk_Window *clicker_window)
{
	struct x11_Window *x11_window = clicker_window;
	XFlush(x11_window->main_display);
}

internal void
init_mouse_event(XEvent *GeneralEvent, struct clk_WindowEvent *event)
{
	if (GeneralEvent->type == MotionNotify) {
		event->val.mouse.x = GeneralEvent->xmotion.x;
		event->val.mouse.y = GeneralEvent->xmotion.y;
		event->val.mouse.button = 0;
	} else {
		event->val.mouse.x = GeneralEvent->xbutton.x;
		event->val.mouse.y = GeneralEvent->xbutton.y;
		event->val.mouse.button = GeneralEvent->xbutton.button > 3 ?
						  3 :
						  GeneralEvent->xbutton.button;
	}
}

void
window_draw_debug_snack(clk_Window *clicker_window, const char *text)
{
	struct x11_Window *x11_window = clicker_window;

	int line_height = 20;
	int x = 10;
	int y = 20;

	const char *p = text;
	const char *newline_loc;

	while ((newline_loc = strchr(p, '\n')) != NULL) {
		int len = newline_loc - p;

		XDrawString(x11_window->main_display, x11_window->main_window,
			    x11_window->context, x, y, p, len);
		y += line_height;
		p = newline_loc + 1; // skip newline character
	}

	XDrawString(x11_window->main_display, x11_window->main_window,
		    x11_window->context, x, y, p, strlen(p));
}
