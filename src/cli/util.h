#pragma once
//#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>

#define TIME_SIZE 100
int get_number_of_processors(void);
size_t time_mu(void);
int time_str(size_t time_mu, char *time_string);
int file_exists(const char* path);
void gd_random_string(char *s, const int len);
int directory_exists(const char* path);
int get_file_version(char* filename, char *version);
char *str_ndup(char *str, int chars);
void str_trim(char *str);
void str_replace(const char *s, const char *oldW, 
	const char *newW, char *result);
int str_contains(const char *str, const char* word);
int str_startswith(const char *str, const char* beg);
int path_concat(const char *s1, const char *s2, char *s3);
//void ShowLastError();
