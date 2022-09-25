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
