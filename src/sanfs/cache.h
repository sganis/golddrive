#pragma once
#include <libssh2_sftp.h>
#include "uthash.h"

#define CACHE_TTL 5 /* secs */

/* cached stat */
typedef struct _CACHE_ATTRIBUTES {
	char path[256];             /* key (string is WITHIN the structure) */
	LIBSSH2_SFTP_ATTRIBUTES *attrs;
	UT_hash_handle hh;         /* makes this structure hashable */
} CACHE_ATTRIBUTES;

extern CACHE_ATTRIBUTES *g_attributes_map;

// cache function
// function fetch(key, ttl) {
//		value <- cache_read(key)
//		if (!value) {
//			value <- recompute_value()
//			cache_write(key, value, ttl)
//		}
//		return value
// }
/* cache function with probabilistic early expiration */
// function fetch(key, ttl, beta = 1) {
//		value, delta, expiry <- cache_read(key)
//		if (!value || time() - delta * beta * log(rand(0, 1)) >= expiry) {
//			start <- time()
//			value <- recompute_value()
//			delta <- time() - start
//			cache_write(key, (value, delta), ttl)
//		}
//		return value
// }


inline void cache_attributes_write(CACHE_ATTRIBUTES *value)
{
	lock();
	HASH_ADD_STR(g_attributes_map, path, value);
	unlock();
}

inline CACHE_ATTRIBUTES * cache_attributes_read(const char* name)
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
