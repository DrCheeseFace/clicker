#include "./internal.h"
#include <stdio.h>
#include <string.h>

global_variable const char *option_to_str[OPTION_FLAGS_COUNT] = {
	"-h", "--help", "-v", "--version"
};

const char *clk_keysym_to_string[CLK_KEYSYM_COUNT_] = {
	"ARROW UP",  "ARROW DOWN", "ARROW LEFT", "ARROW RIGHT",
	"BACKSPACE", "____",	   "NOT FOUND"
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
