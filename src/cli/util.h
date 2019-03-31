#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdarg.h>
#include "jsmn.h"

extern size_t		g_sftp_calls;
extern size_t		g_sftp_cached_calls;
extern char*		g_logfile;

#define STATS		0
#define USE_CACHE	0
#define CACHE_TTL	1000 /* millisecs */

/* logging */
#define ERROR		0
#define WARN		1
#define INFO		2
#define DEBUG		3
#define LOGLEVEL	ERROR

#define log_message(level, format, ...) {								\
	int thread = GetCurrentThreadId();									\
	printf("%zd: %zd: %-6d: %s: %-15s:%3d: ",							\
		g_sftp_calls, time_mu(), thread, level, __func__, __LINE__);	\
	printf(format, __VA_ARGS__);										\
	fflush(stdout);														\
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

typedef struct fs_config {
	char *npath;
	char *host;
	char **hostlist;
	int hostcount;
	char *locuser;
	char *user;
	char *pkey;
	char *drive;
	char *json;
	char *args;
	char *home;
	char *path;
	char letter;
	int port;
	int hidden;
	unsigned local_uid;
	unsigned remote_uid;
} fs_config;

extern fs_config g_fs;

char *strndup(char *str, int chars);
int get_number_of_processors(void);
size_t time_mu(void);
int time_str(size_t time_ms, char *time_string);
void strtrim(char *str);
int file_exists(const char* path);
int jsoneq(const char *json, jsmntok_t *tok, const char *s);
int load_ini(const char *appdir, fs_config *fs);
int load_json(fs_config * fs);
int randint(int min, int max);
void gen_random(char *s, const int len);
void gdlog(const char *fmt, ...);
int directory_exists(const char* path);
