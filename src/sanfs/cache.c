#include "util.h"
#include "cache.h"

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