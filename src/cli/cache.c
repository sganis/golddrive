#include "config.h"
#include "util.h"
#include "cache.h"
#include "uthash.h"

CACHE_STAT* cache_stat_find(const char* path)
{
	if (!path)
		return NULL;
	CACHE_STAT* value = NULL;
	HASH_FIND_STR(g_cache_stat_ht, path, value);
	if (!value) {
		return NULL;
	}
	else if (time_mu() - CACHE_TTL * 1000 >= value->expiry) {
		//debug("CACHE EXPIRED: %s\n", path);
		return NULL;
	}
	else {
		return value;
	}
}

void cache_stat_add(CACHE_STAT *value)
{
	if (!value)
		return;
	CACHE_STAT* entry = NULL;
	HASH_FIND_STR(g_cache_stat_ht, value->path, entry);  /* path already in the hash? */
	cache_stat_lock();
	if (!entry) {
		value->expiry = time_mu() + CACHE_TTL * 1000;
		HASH_ADD_STR(g_cache_stat_ht, path, value);
	}
	else {
		entry->attrs = value->attrs;
		entry->expiry = time_mu() + CACHE_TTL * 1000;
	}
	cache_stat_unlock();
}



