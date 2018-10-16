#pragma once
#include <libssh2_sftp.h>
#include "uthash.h"

/* cached stat */
typedef struct _CACHE_ATTRIBUTES {
	char path[256];             /* key (string is WITHIN the structure) */
	LIBSSH2_SFTP_ATTRIBUTES *attrs;
	UT_hash_handle hh;         /* makes this structure hashable */
} CACHE_ATTRIBUTES;

extern CACHE_ATTRIBUTES *g_attributes_map;

inline void cache_attributes_add(CACHE_ATTRIBUTES *value)
{
	HASH_ADD_STR(g_attributes_map, path, value);
}

inline CACHE_ATTRIBUTES * cache_attributes_find(const char* name)
{
	CACHE_ATTRIBUTES *value = NULL;
	HASH_FIND_STR(g_attributes_map, name, value);
	return value;
}


//int main(int argc, char *argv[]) {
//	const char *names[] = { "joe", "bob", "betty", NULL };
//	struct my_struct *s, *tmp, *users = NULL;
//
//	for (int i = 0; names[i]; ++i) {
//		s = (struct my_struct *)malloc(sizeof *s);
//		strcpy(s->name, names[i]);
//		s->id = i;
//		HASH_ADD_STR(users, name, s);
//	}
//
//	HASH_FIND_STR(users, "betty", s);
//	if (s) printf("betty's id is %d\n", s->id);
//
//	/* free the hash table contents */
//	HASH_ITER(hh, users, s, tmp) {
//		HASH_DEL(users, s);
//		free(s);
//	}
//	return 0;
//}
