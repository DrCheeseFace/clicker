#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE

#include "../internal.h"
#include "./posix_internal.h"

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
	if (end.ns >= start.ns) {
		delta->s = end.s - start.s;
		delta->ns = end.ns - start.ns;
	} else {
		delta->s = end.s - start.s - 1;
		delta->ns = (end.ns + NS_PER_SEC) - start.ns;
	}
}

void
time_sleep_us(uint32_t ms)
{
	usleep(ms);
}

Bool
time_greater_than_or_equal_to(struct clk_Time t1, struct clk_Time t2)
{
	if (t1.s > t2.s)
		return TRUE;
	if (t1.s < t2.s)
		return FALSE;
	return t1.ns >= t2.ns;
}
