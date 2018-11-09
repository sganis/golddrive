#include "util.h"
#include "cache.h"


//void ht_attributes_add(CACHE_ATTRIBUTES *value)
//{
//	CACHE_ATTRIBUTES *exists;
//	HASH_FIND_INT(g_attributes_ht, value->path, exists);  /* path already in the hash? */
//	ht_attributes_lock(1);
//	if (!exists) {
//		value->expiry = time_mu() + CACHE_TTL * 1000;
//		HASH_ADD_STR(g_attributes_ht, path, value);
//	}
//	else {
//		exists->attrs = value->attrs;
//		exists->expiry = time_mu() + CACHE_TTL * 1000;
//	}
//	ht_attributes_lock(0);
//}
//
//CACHE_ATTRIBUTES * ht_attributes_find(const char* path)
//{
//	CACHE_ATTRIBUTES *value = NULL;
//	HASH_FIND_STR(g_attributes_ht, path, value);
//	if (!value) {
//		return NULL;
//	}
//	else if (time_mu() - CACHE_TTL * 1000 >= value->expiry) {
//		debug("CACHE EXPIRED: %s\n", path);
//		return NULL;
//	}
//	else {
//		return value;
//	}
//}
//
//
