#pragma once
#include <stdio.h>
#include <stdarg.h>

#define USE_CACHE	0
#define DEBUG		0
#if DEBUG
#define debug(...) {						\
	int thread_id = GetCurrentThreadId();	\
	printf("%d: DEBUG: %s: %d: ", thread_id, __func__, __LINE__);	\
	printf(__VA_ARGS__);					\
	printf("\n");							\
}
#define debug_cached(...) {						\
	int thread_id = GetCurrentThreadId();	\
	printf("%d: DEBUG: %s: %d: CACHED: ", thread_id, __func__, __LINE__);	\
	printf(__VA_ARGS__);					\
	printf("\n");							\
}
#else
#define debug(...) {}
#define debug_cached(...) {}
#endif


inline void trim_str(char* str, int len)
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