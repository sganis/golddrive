#include "config.h"
#include "util.h"
#include "cache.h"
#include "uthash.h"

CACHE_INODE* cache_inode_find(const char* path)
{
	if (!path)
		return NULL;
	CACHE_INODE* value = NULL;
	HASH_FIND_STR(g_cache_inode_ht, path, value);
	if (!value) {
		return NULL;
	}
	else if (time_mu() - (size_t)CACHE_INODE_TTL * 1000 >= value->expiry) {
		//debug("CACHE EXPIRED: %s\n", path);
		return NULL;
	}
	else {
		return value;
	}
}

void cache_inode_add(CACHE_INODE* value)
{
	if (!value)
		return;
	CACHE_INODE* entry = NULL;
	HASH_FIND_STR(g_cache_inode_ht, value->path, entry);  /* path already in the hash? */
	cache_inode_lock();
	if (!entry) {
		value->expiry = time_mu() + (size_t)CACHE_INODE_TTL * 1000;
		HASH_ADD_STR(g_cache_inode_ht, path, value);
	}
	else {
		entry->inode = value->inode;
		entry->expiry = time_mu() + (size_t)CACHE_INODE_TTL * 1000;
	}
	cache_inode_unlock();
}

