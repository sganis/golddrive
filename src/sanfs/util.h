#pragma once
#include <stdio.h>
#include <stdarg.h>

#define USE_CACHE 1

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
