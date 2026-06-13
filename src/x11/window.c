#include "../window.h"
#include "../utils.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <mr_utils.h>
#include <stdlib.h>
#include <string.h>

internal void init_mouse_event(XEvent GeneralEvent,
			       struct clicker_WindowEvent *event);

global_variable GC context;
global_variable Window root_window;
global_variable Display *main_display;
global_variable Atom wm_delete_window;

void
window_init(void)
{
	main_display = XOpenDisplay(0);
	root_window = XDefaultRootWindow(main_display);
	wm_delete_window = XInternAtom(main_display, "WM_DELETE_WINDOW", False);
	context = XDefaultGC(main_display, 0);
	XSetForeground(main_display, context,
		       WhitePixel(main_display, DefaultScreen(main_display)));
}

void
window_cleanup(void)
{
	XCloseDisplay(main_display);
}

clicker_Window *
window_create(int window_x, int window_y, int window_w, int window_h,
	      int border_w)
{
	int AttributeValueMask = CWBackPixel | CWEventMask;
	XSetWindowAttributes WindowAttributes = { 0 };

	WindowAttributes.event_mask = StructureNotifyMask | ExposureMask |
				      KeyPressMask | KeyReleaseMask |
				      ButtonPressMask | ButtonReleaseMask |
				      PointerMotionMask;

	WindowAttributes.background_pixel = WINDOW_BACKGROUND_COLOR;

	Window main_window =
		XCreateWindow(main_display, root_window, window_x, window_y,
			      window_w, window_h, border_w, CopyFromParent,
			      CopyFromParent, CopyFromParent,
			      AttributeValueMask, &WindowAttributes);

	XStoreName(main_display, main_window, PROGRAM_NAME);

	XSetWMProtocols(main_display, main_window, &wm_delete_window, 1);

	XMapWindow(main_display, main_window);

	clicker_Window *window = malloc(sizeof(Window));
	*(Window *)window = main_window;

	return window;
}

int
window_destroy(clicker_Window *window)
{
	int err = XDestroyWindow(main_display, *(Window *)window);
	free(window);

	return err;
}

void
window_get_event(clicker_Window *window, struct clicker_WindowEvent *event)
{
	XEvent GeneralEvent = { 0 };
	XNextEvent(main_display, &GeneralEvent);

	event->type = CLICKER_WINDOW_EVENT_TYPE_NONE;

	if (GeneralEvent.xany.window != *(Window *)window)
		return;

	switch (GeneralEvent.type) {
	case KeyPress:
		event->type = CLICKER_WINDOW_EVENT_TYPE_KEYDOWN;
		event->val.keycode = GeneralEvent.xkey.keycode;
		break;

	case KeyRelease:
		event->type = CLICKER_WINDOW_EVENT_TYPE_KEYUP;
		event->val.keycode = GeneralEvent.xkey.keycode;
		break;

	case ButtonPress:
		event->type = CLICKER_WINDOW_EVENT_TYPE_MOUSEDOWN;
		init_mouse_event(GeneralEvent, event);
		break;

	case ButtonRelease:
		event->type = CLICKER_WINDOW_EVENT_TYPE_MOUSEUP;
		init_mouse_event(GeneralEvent, event);
		break;

	case MotionNotify:
		event->type = CLICKER_WINDOW_EVENT_TYPE_MOUSEMOVE;
		init_mouse_event(GeneralEvent, event);
		break;

	case ClientMessage: {
		if ((Atom)GeneralEvent.xclient.data.l[0] == wm_delete_window) {
			event->type = CLICKER_WINDOW_EVENT_TYPE_CLOSEREQ;
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
window_clear(clicker_Window *window)
{
	XClearWindow(main_display, *(Window *)window);
}

void
window_flush_display(void)
{
	XFlush(main_display);
}

internal void
init_mouse_event(XEvent GeneralEvent, struct clicker_WindowEvent *event)
{
	event->val.mouse.x = GeneralEvent.xbutton.x_root;
	event->val.mouse.y = GeneralEvent.xbutton.y_root;
	event->val.mouse.button = GeneralEvent.xbutton.button > 3 ?
					  3 :
					  GeneralEvent.xbutton.button;
}

void
window_draw_debug_snack(clicker_Window *window, const char *text)
{
	int line_height = 20;
	int x = 10;
	int y = 20;

	const char *p = text;
	const char *newline_loc;

	while ((newline_loc = strchr(p, '\n')) != NULL) {
		int len = newline_loc - p;
		XDrawString(main_display, *(Window *)window, context, x, y, p,
			    len);
		y += line_height;
		p = newline_loc + 1; // skip newline character
	}

	XDrawString(main_display, *(Window *)window, context, x, y, p,
		    strlen(p));
}
