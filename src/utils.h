#ifndef UTILS_H
#define UTILS_H

#include <mr_utils.h>

#define VERSION "0.0.1"
#define PROGRAM_NAME "clicker"

#define LICENSE                                                                \
	"BSD 3-Clause License\n"                                               \
	"Copyright (c) 2026, Tharun Tilakumara\n"                              \
	"https://opensource.org/license/BSD-2-clause\n"                        \
	"\nWritten by Tharun Tilakumara\n"

enum OptionFlags {
	OPTION_FLAGS_HELP_SHORT,
	OPTION_FLAGS_HELP_LONG,
	OPTION_FLAGS_VERSION_SHORT,
	OPTION_FLAGS_VERSION_LONG,
	OPTION_FLAGS_COUNT,
};

void log_help(void);
void log_version(void);
Err process_arg(char *arg);

#endif
