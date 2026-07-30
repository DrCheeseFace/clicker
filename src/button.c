#include "internal.h"

struct {
	uint8_t length;
	struct clk_Button buttons[16];
} registered_buttons;

mrm_internal struct clk_Button *
button_registered_buttons_append(struct clk_Button button)
{
	registered_buttons.buttons[registered_buttons.length] = button;
	registered_buttons.length++;

	ASSERT(registered_buttons.length <= 16);

	return &registered_buttons.buttons[registered_buttons.length - 1];
}

mrm_internal struct clk_Button *
button_get_registered_button(const char *id_str, int *idx)
{
	struct clk_Button *button;

	for (size_t i = 0; i < registered_buttons.length; i++) {
		button = &registered_buttons.buttons[i];

		if (strcmp(button->id_str, id_str) == 0) {
			if (idx != NULL) {
				*idx = i;
			}
			return button;
		}
	}

	return NULL;
}

void
button_init(void)
{
	memset(&registered_buttons, 0, sizeof(registered_buttons));
}

void
button_destroy(void)
{
	memset(&registered_buttons, 0, sizeof(registered_buttons));
}

Err
button_register_button(const char *id_str, struct clk_Rect box,
		       void (*on_click)(struct clk_EditorState *state,
					void *args),
		       Bool (*destroy_when)(struct clk_EditorState *state),
		       void *args)
{
	struct clk_Button *button = button_get_registered_button(id_str, NULL);

	if (!button) {
		button = button_registered_buttons_append(
			(struct clk_Button){ 0 });
	}

	strcpy(button->id_str, id_str);
	button->box = box;
	button->on_click = on_click;
	button->destroy_when = destroy_when;
	button->args = args;

	return OK;
}

void
button_registered_buttons_purge_dead(struct clk_EditorState *state)
{
	for (int i = registered_buttons.length - 1; i >= 0; i--) {
		struct clk_Button *button = &registered_buttons.buttons[i];

		if (button->destroy_when && button->destroy_when(state)) {
			registered_buttons.buttons[i] =
				registered_buttons
					.buttons[registered_buttons.length - 1];
			memset(&registered_buttons
					.buttons[registered_buttons.length - 1],
			       0, sizeof(struct clk_Button));
			registered_buttons.length--;
		}
	}
}

Bool
button_handle_click(struct clk_EditorState *state,
		    struct clk_MousePosition mouse_position)
{
	button_registered_buttons_purge_dead(state);

	if (registered_buttons.length == 0)
		return FALSE;

	// searched backwards for most recently added button
	struct clk_Button button;
	for (int i = registered_buttons.length - 1; i >= 0; i--) {
		button = registered_buttons.buttons[i];
		if (is_pointer_within_bounds(mouse_position, button.box)) {
			button.on_click(state, button.args);
			return TRUE;
		}
	}
	return FALSE;
}
