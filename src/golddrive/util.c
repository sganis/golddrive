#include "util.h"
#include <Windows.h>
#include <time.h>
#include <string.h>

char *strndup(char *str, int chars)
{
	char *buffer;
	buffer = (char *)malloc(chars + 1);
	if (buffer) {
		memcpy(buffer, str, chars);
		buffer[chars+1] = 0;
	}
	return buffer;
}

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

size_t time_mu(void)
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
	//ret /= 10000; /* From 100 nano seconds (10^-7) to 1 millisecond (10^-3) intervals */
	ret /= 10; /* From 100 nano seconds (10^-7) to 1 microsecond (10^-6) intervals */
	return ret;
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

int jsoneq(const char *json, jsmntok_t *tok, const char *s)
{
	return (tok->type == JSMN_STRING
		&& (int)strlen(s) == tok->end - tok->start
		&& strncmp(json + tok->start, s, tok->end - tok->start) == 0) ? 0 : -1;
}

int load_json(fs_config * fs)
{
	// fill json with json->drive parameters
	if (!file_exists(fs->json)) {
		fprintf(stderr, "cannot read json file: %s\n", fs->json);
		return 1;
	}
	char* JSON_STRING = 0;
	size_t size = 0;
	FILE *fp = fopen(fs->json, "r");
	fseek(fp, 0, SEEK_END); /* Go to end of file */
	size = ftell(fp); /* How many bytes did we pass ? */
	rewind(fp);
	JSON_STRING = malloc((size + 1) * sizeof(*JSON_STRING)); /* size + 1 byte for the \0 */
	fread(JSON_STRING, size, 1, fp); /* Read 1 chunk of size bytes from fp into buffer */
	JSON_STRING[size] = '\0';
	fclose(fp);

	int i, r;
	jsmn_parser p;
	jsmntok_t t[1024]; /* We expect no more than 128 tokens */
	jsmn_init(&p);
	r = jsmn_parse(&p, JSON_STRING, strlen(JSON_STRING), t, sizeof(t) / sizeof(t[0]));
	if (r < 0) {
		fprintf(stderr, "Failed to parse JSON: %d\n", r);
		return 1;
	}

	/* Assume the top-level element is an object */
	if (r < 1 || t[0].type != JSMN_OBJECT) {
		fprintf(stderr, "Object expected, json type=%d\n", t[0].type);
		return 1;
	}
	
	/* Loop over all keys of the root object */
	for (i = 1; i < r; i++) {
		/* only interested in drives key */
		if (jsoneq(JSON_STRING, &t[i], fs->drive) == 0) {
			// drives object
			jsmntok_t *d = &t[i + 1];
			int j;
			int size = t[i + 1].size;
			for (j = 0; j < size; j++) {
				jsmntok_t *k = &t[i + j + 2];
				if (k->type == JSMN_STRING) {
					char * key = strndup(JSON_STRING + k->start, k->end - k->start);
					jsmntok_t *v = &t[i + j + 3];
					if (v->type == JSMN_STRING) {
						char * val = strndup(JSON_STRING + v->start, v->end - v->start);
						//printf("%s : %s\n", key, val);
						if (!fs->user && strcmp(key, "user") == 0)
							fs->user = strdup(val);
						else if (!fs->port && strcmp(key, "port") == 0)
							fs->port = atoi(val);
						else if (strcmp(key, "args") == 0)
							fs->args = strdup(val);
						free(val);
						i++;
					}
					else if (v->type == JSMN_ARRAY) {
						fs->hostcount = v->size;
						fs->hostlist = malloc(v->size);
						for (int u = 0; u < v->size; u++) {
							jsmntok_t *h = &t[i+j+u+4];
							int ssize = h->end - h->start;
							//printf("  * %.*s\n", h->end - h->start, JSON_STRING + h->start); 
							fs->hostlist[u] = strndup(JSON_STRING + h->start, ssize);
							fs->hostlist[u][ssize] = '\0';
							//printf("host %d: %s\n", u+1, fs->hostlist[u]);
							
						}
						//i += t[i + 1].size + 1;

						i = i + v->size + 1;
					}
					free(key);	
					
				}
				
				

			}			
		}
		//if (jsoneq(JSON_STRING, &t[i], "hosts") == 0) {
		//	int j;
		//	if (t[i + 1].type != JSMN_ARRAY)
		//		continue;
		//	for (j = 0; j < t[i + 1].size; j++) {
		//		jsmntok_t *g = &t[i + j + 2];
		//		//json->hosts[j] = _strdup(JSON_STRING + g->start, g->end - g->start);
		//	}
		//	i += t[i + 1].size + 1;
		//}
		//else if (jsoneq(JSON_STRING, &t[i], "port") == 0) {
		//	//json->port = strndup(JSON_STRING + t[i + 1].start, t[i + 1].end - t[i + 1].start);
		//	i++;
		//}
		//else if (jsoneq(JSON_STRING, &t[i], "drive") == 0) {
		//	//json->drive = strndup(JSON_STRING + t[i + 1].start, t[i + 1].end - t[i + 1].start);
		//	i++;
		//}
		//else if (jsoneq(JSON_STRING, &t[i], "path") == 0) {
		//	//json->path = strndup(JSON_STRING + t[i + 1].start, t[i + 1].end - t[i + 1].start);
		//	i++;
		//}
		//else {
		//	// ignore
		//	//printf("Unexpected key: %.*s\n", t[i].end-t[i].start, JSON_STRING + t[i].start);
		//}
	}
	free(JSON_STRING);
	//printf("Hosts:\n");
	//i = 0;
	//while (json->hosts[i])
	//	printf("  - %s\n", json->hosts[i++]);
	//printf("Port: %s\n", json->port);
	//printf("Drive: %s\n", json->drive);
	//printf("Path: %s\n", json->path);
	return 0;
}

int load_ini(const char* appdir, fs_config * fs)
{
	//printf("loading ini..., json.jsonfile=%s\n",json->jsonfile);
	char key[100] = { '\0' };
	char val[FILENAME_MAX] = { '\0' };
	char line[300] = { '\0' };
	size_t span = 0;
	char ini_path[FILENAME_MAX];
	snprintf(ini_path, sizeof ini_path, "%s\\golddrive.ini", appdir);
	//printf("ini path=%s\n", ini_path);
	FILE *ini_file = fopen(ini_path, "r");
	if (!ini_file) {
		fprintf(stderr, "json config will not be used, cannot read ini file: %s\n", ini_path);
		return 1;
	}
	else {
		while (fgets(line, sizeof(line), ini_file)) {
			char *equal = strpbrk(line, "="); //find the equal
			if (equal) {
				span = equal - line;
				memcpy(key, line, span);
				key[span] = '\0';
				strtrim(key);
				if (strcmp(key, "json") == 0) {
					equal++; //advance past the =
					char *nl = strpbrk(equal, "\n"); //fine the newline
					if (nl) {
						span = nl - equal;
						//printf("span=%d\n", span);
						strncpy(val, equal, span);
						strtrim(val);
						fs->json = strdup(val);
					}
					else {
						//printf("no nl\n");
					}
				}
			}
		}
	}
	fclose(ini_file);
	return 0;
}

void strtrim(char *str)
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

void gdlog(const char *fmt, ...)
{
	char message[1000];
	va_list args;
	va_start(args, fmt);
	vsprintf(message, fmt, args);
	va_end(args);
	printf("%s", message);
	if (g_logfile) {
		FILE *f = fopen(g_logfile, "a");
		if (f != NULL)
			fprintf(f, "golddrive: %s", message);
		fclose(f);
	}
}

void gen_random(char *s, const int len)
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