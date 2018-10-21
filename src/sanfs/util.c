#include "util.h"
#include <Windows.h>
#include <time.h>

int time_str(size_t time_ms, char *time_string)
{
	time_t current_time;
	char* c_time_string;
	current_time = time_ms / 1000;
	if (current_time == ((time_t)-1)) {
		(void)fprintf(stderr, "Failure to obtain the current time.\n");
		return -1;
	}
	c_time_string = ctime(&current_time);
	if (c_time_string == NULL) {
		(void)fprintf(stderr, "Failure to convert the current time.\n");
		return -1;
	}
	strcpy(time_string, c_time_string);
	free(c_time_string);
	return 0;
}

size_t time_ms(void)
{
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
}

int get_number_of_processors(void)
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors;
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

int file_exists(const char* path)
{
	DWORD attr = GetFileAttributesA(path);
	return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}