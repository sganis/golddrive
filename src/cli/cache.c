// src/cli/cache.c
#include "config.h"
#include "util.h"
#include "cache.h"
#include "uthash.h"

CACHE_INODE* cache_inode_find(const char* path)
{
	if (!path)
		return NULL;
	CACHE_INODE* value = NULL;
	cache_inode_lock();
	HASH_FIND_STR(g_cache_inode_ht, path, value);
	if (!value) {
		cache_inode_unlock();
		return NULL;
	}
	else if (time_mu() >= value->expiry) {
		cache_inode_unlock();
		return NULL;
	}
	else {
		cache_inode_unlock();
		return value;
	}
}

void cache_inode_add(CACHE_INODE* value)
{
	if (!value)
		return;
	CACHE_INODE* entry = NULL;
	cache_inode_lock();
	HASH_FIND_STR(g_cache_inode_ht, value->path, entry);
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
