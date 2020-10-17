#pragma once
#include <libssh2_sftp.h>
#include "uthash.h"

/* cached generated inode number */
typedef struct {
	char path[MAX_PATH];            /* key (string is WITHIN the structure) */
	unsigned long inode;			/* generated inode number from path */
	size_t expiry;					/* expiration in microsecons */
	UT_hash_handle hh;				/* makes this structure hashable */
} CACHE_INODE;

extern CACHE_INODE* g_cache_inode_ht;
extern SRWLOCK g_cache_inode_lock;

CACHE_INODE* cache_inode_find(const char* path);
void cache_inode_add(CACHE_INODE* inode);


inline void cache_inode_lock() {
	AcquireSRWLockExclusive(&g_cache_inode_lock);
}
inline void cache_inode_unlock() {
	ReleaseSRWLockExclusive(&g_cache_inode_lock);
}

/* cached stat, not used yet */
typedef struct {
	char path[MAX_PATH];            /* key (string is WITHIN the structure) */
	LIBSSH2_SFTP_ATTRIBUTES attrs;	/* stats */
	size_t expiry;					/* expiration in microsecons */
	UT_hash_handle hh;				/* makes this structure hashable */
} CACHE_STAT;

extern CACHE_STAT * g_cache_stat_ht;
extern SRWLOCK g_cache_stat_lock;
extern size_t g_cache_calls;

CACHE_STAT* cache_stat_find(const char* name);
void cache_stat_add(CACHE_STAT* value);


inline void cache_stat_lock() {
	AcquireSRWLockExclusive(&g_cache_stat_lock);
}
inline void cache_stat_unlock() {
	ReleaseSRWLockExclusive(&g_cache_stat_lock);
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
