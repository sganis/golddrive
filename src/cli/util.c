#include <windows.h>
#include <strsafe.h>
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
	// 2020-04-26 13:59:09
	ts = *localtime(&current_time);
	strftime(time_string, TIME_SIZE, "%Y-%m-%d %H:%M:%S", &ts);
	return 0;
}

size_t time_mu(void)
{
	FILETIME ft;
	LARGE_INTEGER li;
	/* Get the amount of 100 nano seconds intervals elapsed since January 1, 1601 (UTC) and copy it
	* to a LARGE_INTEGER structure. */
	GetSystemTimeAsFileTime(&ft);
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;
	long long ret = li.QuadPart;
	ret -= 116444736000000000LL; /* Convert from file time to UNIX epoch time. */
	//ret /= 10000; /* From 100 nano seconds (10^-7) to 1 millisecond (10^-3) intervals */
	ret /= 10; /* From 100 nano seconds (10^-7) to 1 microsecond (10^-) intervals */
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



//int load_ini(const char* appdir, GDCONFIG * fs)
//{
//	//printf("loading ini..., json.jsonfile=%s\n",json->jsonfile);
//	char key[100] = { '\0' };
//	char val[FILENAME_MAX] = { '\0' };
//	char line[300] = { '\0' };
//	size_t span = 0;
//	char ini_path[FILENAME_MAX];
//	snprintf(ini_path, sizeof ini_path, "%s\\golddrive.ini", appdir);
//	//printf("ini path=%s\n", ini_path);
//	FILE *ini_file = fopen(ini_path, "r");
//	if (!ini_file) {
//		fprintf(stderr, "json config will not be used, cannot read ini file: %s\n", ini_path);
//		return 1;
//	}
//	else {
//		while (fgets(line, sizeof(line), ini_file)) {
//			char *equal = strpbrk(line, "="); //find the equal
//			if (equal) {
//				span = equal - line;
//				memcpy(key, line, span);
//				key[span] = '\0';
//				str_trim(key);
//				if (strcmp(key, "json") == 0) {
//					equal++; //advance past the =
//					char *nl = strpbrk(equal, "\n"); //fine the newline
//					if (nl) {
//						span = nl - equal;
//						//printf("span=%d\n", span);
//						strncpy(val, equal, span);
//						str_trim(val);
//						fs->json = strdup(val);
//					}
//					else {
//						//printf("no nl\n");
//					}
//				}
//			}
//		}
//	}
//	fclose(ini_file);
//	return 0;
//}

void str_trim(char *str)
{
	char *end;

	// Trim leading space
	while (isspace((unsigned char)*str))
		str++;

	if (*str == 0)  // All spaces?
		return;

	// Trim trailing space
	end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end))
		end--;

	// Write new null terminator character
	end[1] = '\0';
}

int randint(int min, int max)
{
	srand((unsigned int)time(NULL));
	int n = min + (rand() % (max-min+1));
	return n;
}	



void gd_random_string(char *s, const int len)
{
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";
	for (int i = 0; i < len; ++i) {
		s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
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

void str_replace(const char *s, const char *oldW, const char *newW, char *result)
{
	// result must be char result[MAX_PATH]
	int i, cnt = 0;
	int newWlen = (int)strlen(newW);
	int oldWlen = (int)strlen(oldW);

	// Counting the number of times old word 
	// occur in the string 
	for (i = 0; s[i] != '\0'; i++)
	{
		if (strstr(&s[i], oldW) == &s[i])
		{
			cnt++;

			// Jumping to index after the old word. 
			i += oldWlen - 1;
		}
	}

	// Making new string of enough length 
	//result = (char *)malloc(i + cnt * (newWlen - oldWlen) + 1);

	i = 0;
	while (*s)
	{
		// compare the substring with the result 
		if (strstr(s, oldW) == s)
		{
			strcpy(&result[i], newW);
			i += newWlen;
			s += oldWlen;
		}
		else
			result[i++] = *s++;
	}

	result[i] = '\0';
	//return result;
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

//void ShowLastError()
//{
//	// Retrieve the system error message for the last-error code
//
//	LPVOID lpMsgBuf;
//	LPVOID lpDisplayBuf;
//	DWORD dw = GetLastError();
//
//	FormatMessage(
//		FORMAT_MESSAGE_ALLOCATE_BUFFER |
//		FORMAT_MESSAGE_FROM_SYSTEM |
//		FORMAT_MESSAGE_IGNORE_INSERTS,
//		NULL,
//		dw,
//		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
//		(LPTSTR)&lpMsgBuf,
//		0, NULL);
//
//	// Display the error message and exit the process
//
//	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
//		(lstrlen((LPCTSTR)lpMsgBuf) +  40) * sizeof(TCHAR));
//	StringCchPrintf((LPTSTR)lpDisplayBuf,
//		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
//		TEXT("Function failed with error %d: %s"),
//		dw, lpMsgBuf);
//	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);
//
//	LocalFree(lpMsgBuf);
//	LocalFree(lpDisplayBuf);
//	ExitProcess(dw);
//}
