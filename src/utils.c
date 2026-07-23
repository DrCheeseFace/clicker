#include "./internal.h"

global_variable const char *option_to_str[OPTION_FLAGS_COUNT] = {
	"-h", "--help", "-v", "--version"
};

const char *clk_keysym_to_string[CLK_KEYSYM_COUNT_] = {
	"ARROW UP",   "ARROW DOWN", "ARROW LEFT", "ARROW RIGHT", "EQUALS",
	"MINUS",      "BACKSPACE",  "LEFT SHIFT", "LEFT CTRL",	 "ESCAPE",
	"DEBUG BIND", "____",	    "NOT FOUND"
};

void
log_help(void)

{
	printf("mouse focused text editor\n"
	       "Usage: %s [OPTION] FILE\n"
	       "Options:\n"
	       "  -h, --help         Print help\n"
	       "  -v, --version      Print version\n",
	       PROGRAM_NAME);
}

void
log_version(void)
{
	printf("%s v%s\n"
	       "%s",
	       PROGRAM_NAME, VERSION, LICENSE);
}

void
log_invalid_arg(char *arg)
{
	printf("error: invalid argument "
	       "\"%s\"",
	       arg);
}

Err
process_arg(char *arg)
{
	enum OptionFlags option = OPTION_FLAGS_COUNT;
	for (int i = 0; i < OPTION_FLAGS_COUNT; i++) {
		if (strcmp(arg, option_to_str[i]) == 0) {
			option = i;
			break;
		}
	}

	switch (option) {
	case OPTION_FLAGS_HELP_SHORT:
	case OPTION_FLAGS_HELP_LONG:
		log_help();
		break;
	case OPTION_FLAGS_VERSION_SHORT:
	case OPTION_FLAGS_VERSION_LONG:
		log_version();
		break;
	case OPTION_FLAGS_COUNT:
		log_invalid_arg(arg);
		return ERR;
	default:
		break;
	}
	return OK;
}

Bool
utf8_is_continuation_byte(char byte)
{
	/* 0x80 = 0b10000000  continuation byte marker*/
	/* 0xC0 = 0b11000000  mask*/
	return ((uint8_t)byte & 0xC0) == 0x80;
}

void
utf8_seek_next(char **ptr)
{
	(*ptr)++;

	while (utf8_is_continuation_byte(**ptr)) {
		(*ptr)++;
	}
}

void
debug_save_buffer_to_file(Buffer *buffer, const char *filepath)
{
	FILE *dump_to = fopen(filepath, "w+");

	fwrite(buffer->text, BUFFER_MAX_TEXT_BYTES_LENGTH(buffer->size),
	       sizeof(char), dump_to);

	fclose(dump_to);
}

size_t
get_row_length(BufferID bufferid, size_t row, uint8_t tab_spaces)
{
	Buffer *const buffer = buffers[bufferid];

	char *ptr = buffer_get_ptr_of_line(bufferid, row);

	size_t col_count = 0;
	while (ptr <
	       buffer->text + BUFFER_MAX_TEXT_BYTES_LENGTH(buffer->size)) {
		if (*ptr == UTF8_NEWLINE || *ptr == UTF8_RETURN) {
			break;
		}
		if (*ptr == UTF8_TAB) {
			col_count += tab_spaces;
		} else {
			col_count++;
		}

		buffer_seek_next_utf8(buffer, &ptr);
	}

	return col_count;
}
