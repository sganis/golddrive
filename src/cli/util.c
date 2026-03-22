// src/cli/util.c
#include <windows.h>
#include <strsafe.h>
#include <ntsecapi.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include "util.h"

char *str_ndup(char *str, int chars)
{
	char *buffer;
	buffer = (char *)malloc(chars + 1);
	if (buffer) {
		memcpy(buffer, str, chars);
		buffer[chars] = 0;
	}
	return buffer;
}

int time_str(size_t time_mu, char *time_string)
{
	time_t current_time;
	current_time = time_mu / 1000000;
	struct tm  ts;
	ts = *localtime(&current_time);
	strftime(time_string, TIME_SIZE, "%Y-%m-%d %H:%M:%S", &ts);
	return 0;
}

size_t time_mu(void)
{
	FILETIME ft;
	LARGE_INTEGER li;
	/* Get the amount of 100 nano seconds intervals elapsed since January 1, 1601 (UTC)
	 * and copy it to a LARGE_INTEGER structure. */
	GetSystemTimeAsFileTime(&ft);
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;
	long long ret = li.QuadPart;
	ret -= 116444736000000000LL; /* Convert from file time to UNIX epoch time. */
	ret /= 10; /* From 100 nano seconds (10^-7) to 1 microsecond (10^-6) intervals */
	return (size_t)ret;
}

int get_number_of_processors(void)
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors;
}

int file_exists(const char* path)
{
	DWORD attr = GetFileAttributesA(path);
	return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

void str_trim(char *str)
{
	char *end;

	/* trim leading space */
	while (isspace((unsigned char)*str))
		str++;

	if (*str == 0)
		return;

	/* trim trailing space */
	end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end))
		end--;

	end[1] = '\0';
}

void gd_random_string(char *s, const int len)
{
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";
	unsigned char buf[256];
	int n = len < (int)sizeof(buf) ? len : (int)sizeof(buf);
	/* use OS cryptographic random */
	if (!RtlGenRandom(buf, n)) {
		/* fallback: seed rand and use it */
		srand((unsigned)GetTickCount());
		for (int i = 0; i < len; ++i)
			s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	} else {
		for (int i = 0; i < len; ++i)
			s[i] = alphanum[buf[i % n] % (sizeof(alphanum) - 1)];
	}
	s[len] = 0;
}

int directory_exists(const char* path)
{
	DWORD attr = GetFileAttributesA(path);
	return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
}

int get_file_version(char* filename, char *version)
{
	int rc = 1;
	DWORD  verHandle = 0;
	UINT   size = 0;
	LPBYTE lpBuffer = NULL;
	DWORD  verSize = 0;
	verSize = GetFileVersionInfoSizeA(filename, &verHandle);

	if (verSize)
	{
		LPSTR verData = malloc(verSize);

		if (GetFileVersionInfoA(filename, verHandle, verSize, verData))
		{
			if (VerQueryValueA(verData, "\\", (VOID FAR* FAR*)&lpBuffer, &size))
			{
				if (size)
				{
					VS_FIXEDFILEINFO *verInfo = (VS_FIXEDFILEINFO *)lpBuffer;
					if (verInfo->dwSignature == 0xfeef04bd)
					{
						sprintf_s(version, 1000, "%d.%d.%d.%d",
							(verInfo->dwFileVersionMS >> 16) & 0xffff,
							(verInfo->dwFileVersionMS >> 0) & 0xffff,
							(verInfo->dwFileVersionLS >> 16) & 0xffff,
							(verInfo->dwFileVersionLS >> 0) & 0xffff
						);
						rc = 0;
					}
				}
			}
		}
		free(verData);
	}
	return rc;
}

void str_replace(const char *s, const char *oldW, const char *newW, char *result, size_t result_size)
{
	int i = 0;
	int newWlen = (int)strlen(newW);
	int oldWlen = (int)strlen(oldW);
	int limit = (int)result_size - 1;

	while (*s)
	{
		if (strstr(s, oldW) == s)
		{
			if (i + newWlen > limit)
				break;
			memcpy(&result[i], newW, newWlen);
			i += newWlen;
			s += oldWlen;
		}
		else {
			if (i >= limit)
				break;
			result[i++] = *s++;
		}
	}

	result[i] = '\0';
}

int str_contains(const char *str, const char* word)
{
	return strstr(str, word) != NULL;
}

int str_startswith(const char *str, const char* beg)
{
	size_t lenbeg = strlen(beg);
	size_t lenstr = strlen(str);
	return lenstr < lenbeg ? 0 : strncmp(beg, str, lenbeg) == 0;
}

int path_concat(const char *s1, const char *s2, char *s3)
{
	strcpy_s(s3, MAX_PATH, s1);
	int pathlen = (int)strlen(s1);
	if (!(pathlen > 0 && s1[pathlen - 1] == '/'))
		strcat_s(s3, MAX_PATH, "/");
	strcat_s(s3, MAX_PATH, s2);
	return 0;
}

unsigned long hash_path(const char* path)
{
	unsigned long hash = 5381;
	int c;
	const char* p = path;
	while (c = *p++)
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	return hash;
}
