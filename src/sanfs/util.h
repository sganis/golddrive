#pragma once
#include <stdio.h>
#include <stdarg.h>

#define USE_CACHE	0
#define STATS		0
#define ERROR		0
#define WARN		1
#define INFO		2
#define DEBUG		3
#define LOGLEVEL	ERROR

#define log_message(level, format, ...) {						\
	int thread = GetCurrentThreadId();							\
	printf("%zd: %d: %s: %s: %d: ",								\
		time_ms(), thread, level, __func__, __LINE__);			\
	printf(format, __VA_ARGS__);								\
}
#define debug(format, ...)  log_message("DEBUG", format, __VA_ARGS__) 
#define info(format, ...)   log_message("INFO ", format, __VA_ARGS__) 
#define warn(format, ...)   log_message("WARN ", format, __VA_ARGS__) 
#define error(format, ...)  log_message("ERROR", format, __VA_ARGS__) 

#if LOGLEVEL < DEBUG
#undef debug
#define debug(format, ...) {}
#endif
#if LOGLEVEL < INFO
#undef info
#define info(format, ...) {}
#endif
#if LOGLEVEL < WARN
#undef warn
#define warn(format, ...) {}
#endif

int get_number_of_processors(void);
size_t time_ms(void);
int time_str(size_t time_ms, char *time_string);
void trim_str(char* str, int len);
int file_exists(const char* path);
