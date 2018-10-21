#pragma once
#include <stdio.h>
#include <stdarg.h>

#define USE_CACHE	0
#define STATS		0
#define DEBUG		0
#if DEBUG

#define debug(format, ...) {										\
	int thread_id = GetCurrentThreadId();							\
	printf("%zd: %d: DEBUG: %s: %d: ",								\
		time_ms(), thread_id, __func__, __LINE__);					\
	printf(format, __VA_ARGS__);									\
}
#define debug_cached(format, ...) {									\
	int thread_id = GetCurrentThreadId();							\
	printf("%d: DEBUG: %s: %d: CACHED: ", thread_id, __func__, __LINE__);	\
	printf(format, __VA_ARGS__);									\
}
#else
#define debug(...) {}
#define debug_cached(...) {}
#endif

int get_number_of_processors(void);
size_t time_ms(void);
void trim_str(char* str, int len);