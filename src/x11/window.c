#include "../internal.h"
#include "x11_internal.h"

const XID x11_keycode_to_clk_keysym_map[CLK_KEYSYM_COUNT] = {
	XK_Up,	      XK_Down,	  XK_Left,	XK_Right,  XK_equal, XK_minus,
	XK_BackSpace, XK_Shift_L, XK_Control_L, XK_Escape, XK_bar

};

mrm_internal enum clk_Keysym
window_translate_x11_keycode_to_clk_keysym(struct x11_Window *x11_window,
					   XEvent event)
{
	KeySym keysym =
		XkbKeycodeToKeysym(x11_window->main_display, event.xkey.keycode,
				   0, event.xkey.state & ShiftMask ? 1 : 0);

	if (keysym == NoSymbol) {
		return CLK_KEYSYM_NOT_FOUND;
	}

	for (enum clk_Keysym sym = 0; sym < CLK_KEYSYM_COUNT; sym++) {
		if (keysym == x11_keycode_to_clk_keysym_map[sym]) {
			return sym;
		}
	}

	return CLK_KEYSYM_NOT_FOUND;
}

mrm_internal void
window_inputs_add_input(struct clk_Keystate *keystate, struct clk_Input input)
{
	if (keystate->inputs_len >= MAX_INPUTS) {
		editor_set_err_msg(&clicker_state, "failed to add input");
		return;
	}

	keystate->inputs[keystate->inputs_len] = input;

	keystate->inputs_len++;
}

mrm_internal void
window_inputs_remove_keyboard_input(struct clk_Keystate *keystate,
				    enum clk_Keysym keysym)
{
	for (struct clk_Input *i = &keystate->inputs[0];
	     i < &keystate->inputs[keystate->inputs_len]; i++) {
		if (i->type == CLK_INPUT_TYPE_KEYBOARD &&
		    i->input.key.keysym == keysym) {
			*i = keystate->inputs[keystate->inputs_len - 1];
			keystate->inputs_len -= 1;

			return;
		}
	}
}

mrm_internal void
window_inputs_remove_mouse_input(struct clk_Keystate *keystate,
				 enum clk_EventMouseButton button)
{
	for (struct clk_Input *i = &keystate->inputs[0];
	     i < &keystate->inputs[keystate->inputs_len]; i++) {
		if (i->type == CLK_INPUT_TYPE_MOUSE &&
		    i->input.mouse_button == button) {
			*i = keystate->inputs[keystate->inputs_len - 1];
			keystate->inputs_len -= 1;

			return;
		}
	}
}

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
window_pol_event(void)
{
	struct x11_Window *const x11_window =
		clicker_renderer.clk_window.window_ctx;

	while (XPending(x11_window->main_display) > 0) {
		XEvent GeneralEvent = { 0 };
		XNextEvent(x11_window->main_display, &GeneralEvent);

		time_get_time(&clicker_keystate.current_pol_time);

		if (XFilterEvent(&GeneralEvent, x11_window->main_window))
			continue;

		if (GeneralEvent.xany.window != x11_window->main_window)
			continue;

		switch (GeneralEvent.type) {
		case KeyPress: {
			struct clk_Input input = {
				.type = CLK_INPUT_TYPE_KEYBOARD,
				.input.key.keysym =
					window_translate_x11_keycode_to_clk_keysym(
						x11_window, GeneralEvent),
			};

			KeySym keysym = NoSymbol;
			int count = Xutf8LookupString(
				x11_window->xic, &GeneralEvent.xkey,
				input.input.key.utf8,
				sizeof(input.input.key.utf8) - 1, &keysym,
				NULL);
			if (count < 0 ||
			    count >= (int)sizeof(input.input.key.utf8)) {
				count = 0;
			}

			input.input.key.utf8[count] = '\0';
			input.time = clicker_keystate.current_pol_time;

			window_inputs_add_input(&clicker_keystate, input);
			break;
		}

		case KeyRelease: {
			if (XPending(x11_window->main_display) > 0) {
				XEvent next;
				XPeekEvent(x11_window->main_display, &next);
				if (next.type == KeyPress &&
				    next.xkey.keycode ==
					    GeneralEvent.xkey.keycode) {
					// fake release
					break;
				}
			}

			enum clk_Keysym keysym =
				window_translate_x11_keycode_to_clk_keysym(
					x11_window, GeneralEvent);

			if (keysym != CLK_KEYSYM_NOT_FOUND) {
				window_inputs_remove_keyboard_input(
					&clicker_keystate, keysym);
			}
			break;
		}

		case ButtonPress: {
			struct clk_Input input = {
				.type = CLK_INPUT_TYPE_MOUSE,
				.input.mouse_button =
					GeneralEvent.xbutton.button
			};
			window_inputs_add_input(&clicker_keystate, input);
			break;
		}

		case ButtonRelease: {
			window_inputs_remove_mouse_input(
				&clicker_keystate, GeneralEvent.xbutton.button);
			break;
		}

		case MotionNotify: {
			XEvent motion_event = GeneralEvent;
			while (XCheckTypedWindowEvent(x11_window->main_display,
						      x11_window->main_window,
						      MotionNotify,
						      &GeneralEvent)) {
			}
			clicker_keystate.mouse_x = motion_event.xmotion.x;
			clicker_keystate.mouse_y = motion_event.xmotion.y;
			break;
		}

		case ClientMessage: {
			if ((Atom)GeneralEvent.xclient.data.l[0] ==
			    x11_window->wm_delete_window) {
				window_inputs_add_input(
					&clicker_keystate,
					(struct clk_Input){
						.type = CLK_INPUT_TYPE_CLOSEREQ });
			}
			break;
		}

		case ConfigureNotify: {
			window_inputs_add_input(
				&clicker_keystate,
				(struct clk_Input){
					.type = CLK_INPUT_TYPE_RESIZEREQ });
			break;
		}

		case EnterNotify:
		case LeaveNotify:
		default:
			break;
		}
	}
}

int
window_inputs_contains_input(struct clk_Keystate keystate,
			     struct clk_Input input)
{
	for (struct clk_Input *i = &keystate.inputs[0];
	     i < &keystate.inputs[keystate.inputs_len]; i++) {
		if (i->type != input.type)
			continue;

		switch (i->type) {
		case CLK_INPUT_TYPE_KEYBOARD:
			if (i->input.key.keysym != input.input.key.keysym)
				break;
			if (i->input.key.keysym == CLK_KEYSYM_NOT_FOUND) {
				if (strcmp(i->input.key.utf8,
					   input.input.key.utf8) == 0)
					return TRUE;
			} else {
				return TRUE;
			}
			break;
		case CLK_INPUT_TYPE_MOUSE:
			if (i->input.mouse_button == input.input.mouse_button)
				return TRUE;
			break;
		case CLK_INPUT_TYPE_CLOSEREQ:
		case CLK_INPUT_TYPE_RESIZEREQ:
			return TRUE;
		default:
			break;
		}
	}

	return FALSE;
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
