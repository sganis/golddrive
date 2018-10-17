#include "util.h"
#include <Windows.h>

size_t time_ms()
{
#ifdef _WIN32
	/* Windows */
	FILETIME ft;
	LARGE_INTEGER li;

	/* Get the amount of 100 nano seconds intervals elapsed since January 1, 1601 (UTC) and copy it
	* to a LARGE_INTEGER structure. */
	GetSystemTimeAsFileTime(&ft);
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;

	size_t ret = li.QuadPart;
	ret -= 116444736000000000LL; /* Convert from file time to UNIX epoch time. */
	ret /= 10000; /* From 100 nano seconds (10^-7) to 1 millisecond (10^-3) intervals */

	return ret;
#else
	/* Linux */
	struct timeval tv;

	gettimeofday(&tv, NULL);

	uint64 ret = tv.tv_usec;
	/* Convert from micro seconds (10^-6) to milliseconds (10^-3) */
	ret /= 1000;

	/* Adds the seconds (10^0) after converting them to milliseconds (10^-3) */
	ret += (tv.tv_sec * 1000);

	return ret;
#endif
}

void trim_str(char* str, int len)
{
	char *t;
	str[len - 1] = '\0';
	// trim trailing space
	for (t = str + len; --t >= str; )
		if (*t == ' ' || *t == '\n' || *t == '\r' || *t == '\t')
			*t = '\0';
		else
			break;
	// trim leading space
	for (t = str; t < str + len; ++t)
		if (*t != ' ' && *t == '\n' && *t == '\r' && *t == '\t')
			break;

}