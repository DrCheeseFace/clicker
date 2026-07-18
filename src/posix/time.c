#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE

#include "../internal.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>

void
time_get_time(struct clk_Time *time)
{
	struct timespec timespec;
	clock_gettime(CLOCK_MONOTONIC, &timespec);

	time->ns = timespec.tv_nsec;
	time->s = timespec.tv_sec;
}

void
time_get_delta(struct clk_Time start, struct clk_Time end,
	       struct clk_Time *delta)
{
	delta->s = end.s - start.s;

	if (end.ns > start.ns) {
		delta->ns = end.ns - start.ns;
	} else {
		delta->s--;
		end.ns += 1000000000ULL;
		delta->ns = end.ns - start.ns;
	}
}

void
time_sleep_us(uint32_t ms)
{
	usleep(ms);
}
