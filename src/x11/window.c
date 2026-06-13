#include "../window.h"
#include <X11/Xlib.h>
#include <mr_utils.h>
#include <stdlib.h>

//TODO: move this into proper place
Window RootWindow;
Display *MainDisplay;

void
window_init(void)
{
	MainDisplay = XOpenDisplay(0);
	RootWindow = XDefaultRootWindow(MainDisplay);
}

clicker_Window *
window_create(int window_x, int window_y, int window_w, int window_h,
	      int border_w)
{
	int AttributeValueMask = CWBackPixel;
	XSetWindowAttributes WindowAttributes = { 0 };

	// TODO for handling events later
	// WindowAttributes.event_mask = StructureNotifyMask | KeyPressMask |
	// 			      KeyReleaseMask | ExposureMask;

	WindowAttributes.background_pixel = WINDOW_BACKGROUND_COLOR;

	Window MainWindow =
		XCreateWindow(MainDisplay, RootWindow, window_x, window_y,
			      window_w, window_h, border_w, CopyFromParent,
			      CopyFromParent, CopyFromParent,
			      AttributeValueMask, &WindowAttributes);

	XMapWindow(MainDisplay, MainWindow);
	XStoreName(MainDisplay, MainWindow, "THIS IS my NEW WINDOW TITLE");

	//TODO: REMOVE ME when impl event loop
	XFlush(MainDisplay);
	//TODO: impl event loop

	// for (;;) {
	// 	XEvent GeneralEvent = { 0 };
	// 	XNextEvent(MainDisplay, &GeneralEvent);
	// }
	clicker_Window *window = malloc(sizeof(Window));
	return window;
}

int
window_destroy(clicker_Window *window)
{
	int err = XDestroyWindow(MainDisplay, *(Window *)window);
	free(window);

	return err;
}

// extern int XDestroyWindow(Display * /* display */, Window /* w */
// );
