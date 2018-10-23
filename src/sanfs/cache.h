#pragma once
#include <libssh2_sftp.h>
#include "uthash.h"


/* cached stat */
typedef struct {
	char path[MAX_PATH];            /* key (string is WITHIN the structure) */
	LIBSSH2_SFTP_ATTRIBUTES attrs;	/* stats */
	size_t expiry;					/* expiration in microsecons */
	UT_hash_handle hh;				/* makes this structure hashable */
} CACHE_ATTRIBUTES;

extern CACHE_ATTRIBUTES *g_attributes_ht;
extern CRITICAL_SECTION g_attributes_lock;

void ht_attributes_add(CACHE_ATTRIBUTES *value);
CACHE_ATTRIBUTES * ht_attributes_find(const char* name);
inline void ht_attributes_lock(int lock) {
	lock ? EnterCriticalSection(&g_attributes_lock) : 
		LeaveCriticalSection(&g_attributes_lock);
}

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
