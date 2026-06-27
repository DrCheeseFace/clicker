#ifndef X11_INTENRAL_H
#define X11_INTENRAL_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo.h>

struct x11_Window {
	GC context;
	Window root_window;
	Window main_window;
	Display *main_display;
	Atom wm_delete_window;
};

#endif //X11_INTENRAL_H
