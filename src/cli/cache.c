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
	else if (time_mu() - CACHE_INODE_TTL * 1000 >= value->expiry) {
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
		value->expiry = time_mu() + CACHE_INODE_TTL * 1000;
		HASH_ADD_STR(g_cache_inode_ht, path, value);
	}
	else {
		entry->inode = value->inode;
		entry->expiry = time_mu() + CACHE_INODE_TTL * 1000;
	}
	cache_inode_unlock();
}


CACHE_STAT* cache_stat_find(const char* path)
{
	if (!path)
		return NULL;
	CACHE_STAT* value = NULL;
	HASH_FIND_STR(g_cache_stat_ht, path, value);
	if (!value) {
		return NULL;
	}
	else if (time_mu() - CACHE_STAT_TTL * 1000 >= value->expiry) {
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
		value->expiry = time_mu() + CACHE_STAT_TTL * 1000;
		HASH_ADD_STR(g_cache_stat_ht, path, value);
	}
	else {
		entry->attrs = value->attrs;
		entry->expiry = time_mu() + CACHE_STAT_TTL * 1000;
	}
	cache_stat_unlock();
}



#define DEFAULT_CACHE_TIMEOUT_SECS 20
#define DEFAULT_MAX_CACHE_SIZE 10000
#define DEFAULT_CACHE_CLEAN_INTERVAL_SECS 60
#define DEFAULT_MIN_CACHE_CLEAN_INTERVAL_SECS 5

struct cache {
	int on;
	unsigned int stat_timeout_secs;
	unsigned int dir_timeout_secs;
	unsigned int link_timeout_secs;
	unsigned int max_size;
	unsigned int clean_interval_secs;
	unsigned int min_clean_interval_secs;
	struct fuse_operations* next_oper;
	//GHashTable* table;
	//pthread_mutex_t lock;
	time_t last_cleaned;
	uint64_t write_ctr;
};

static struct cache cache;

struct node {
	struct stat stat;
	time_t stat_valid;
	char** dir;
	time_t dir_valid;
	char* link;
	time_t link_valid;
	time_t valid;
};

struct readdir_handle {
	const char* path;
	void* buf;
	fuse_fill_dir_t filler;
	//GPtrArray* dir;
	uint64_t wrctr;
};

struct file_handle {
	/* Did we send an open request to the underlying fs? */
	int is_open;

	/* If so, this will hold its handle */
	unsigned long fs_fh;
};
//
//static void free_node(gpointer node_)
//{
//	struct node* node = (struct node*)node_;
//	g_strfreev(node->dir);
//	g_free(node);
//}

static int cache_clean_entry(void* key_, struct node* node, time_t* now)
{
	(void)key_;
	if (*now > node->valid)
		return TRUE;
	else
		return FALSE;
}

static void cache_clean(void)
{
	/*time_t now = time(NULL);
	if (now > cache.last_cleaned + cache.min_clean_interval_secs &&
		(g_hash_table_size(cache.table) > cache.max_size ||
			now > cache.last_cleaned + cache.clean_interval_secs)) {
		g_hash_table_foreach_remove(cache.table,
			(GHRFunc)cache_clean_entry, &now);
		cache.last_cleaned = now;
	}*/
}

static struct node* cache_lookup(const char* path)
{
	//return (struct node*)g_hash_table_lookup(cache.table, path);
}

static void cache_purge(const char* path)
{
	//g_hash_table_remove(cache.table, path);
}

static void cache_purge_parent(const char* path)
{
	/*const char* s = strrchr(path, '/');
	if (s) {
		if (s == path)
			g_hash_table_remove(cache.table, "/");
		else {
			char* parent = g_strndup(path, s - path);
			cache_purge(parent);
			g_free(parent);
		}
	}*/
}

void cache_invalidate(const char* path)
{
	//pthread_mutex_lock(&cache.lock);
	cache_purge(path);
	//pthread_mutex_unlock(&cache.lock);
}

static void cache_invalidate_write(const char* path)
{
	//pthread_mutex_lock(&cache.lock);
	cache_purge(path);
	cache.write_ctr++;
	//pthread_mutex_unlock(&cache.lock);
}

static void cache_invalidate_dir(const char* path)
{
	//pthread_mutex_lock(&cache.lock);
	cache_purge(path);
	cache_purge_parent(path);
	//pthread_mutex_unlock(&cache.lock);
}

static int cache_del_children(const char* key, void* val_, const char* path)
{
	(void)val_;
	if (strncmp(key, path, strlen(path)) == 0)
		return TRUE;
	else
		return FALSE;
}

static void cache_do_rename(const char* from, const char* to)
{
	//pthread_mutex_lock(&cache.lock);
	//g_hash_table_foreach_remove(cache.table, (GHRFunc)cache_del_children,
	//	(char*)from);
	cache_purge(from);
	cache_purge(to);
	cache_purge_parent(from);
	cache_purge_parent(to);
	//pthread_mutex_unlock(&cache.lock);
}

static struct node* cache_get(const char* path)
{
	struct node* node = cache_lookup(path);
	if (node == NULL) {
		//char* pathcopy = g_strdup(path);
		//node = g_new0(struct node, 1);
		//g_hash_table_insert(cache.table, pathcopy, node);
	}
	return node;
}

void cache_add_attr(const char* path, const struct stat* stbuf, uint64_t wrctr)
{
	struct node* node;

	//pthread_mutex_lock(&cache.lock);
	if (wrctr == cache.write_ctr) {
		node = cache_get(path);
		node->stat = *stbuf;
		node->stat_valid = time(NULL) + cache.stat_timeout_secs;
		if (node->stat_valid > node->valid)
			node->valid = node->stat_valid;
		cache_clean();
	}
	//pthread_mutex_unlock(&cache.lock);
}

static void cache_add_dir(const char* path, char** dir)
{
	struct node* node;

	//pthread_mutex_lock(&cache.lock);
	node = cache_get(path);
	//g_strfreev(node->dir);
	node->dir = dir;
	node->dir_valid = time(NULL) + cache.dir_timeout_secs;
	if (node->dir_valid > node->valid)
		node->valid = node->dir_valid;
	cache_clean();
	//pthread_mutex_unlock(&cache.lock);
}

static size_t my_strnlen(const char* s, size_t maxsize)
{
	const char* p;
	for (p = s; maxsize && *p; maxsize--, p++);
	return p - s;
}

static void cache_add_link(const char* path, const char* link, size_t size)
{
	struct node* node;

	//pthread_mutex_lock(&cache.lock);
	node = cache_get(path);
	//g_free(node->link);
	//node->link = g_strndup(link, my_strnlen(link, size - 1));
	node->link_valid = time(NULL) + cache.link_timeout_secs;
	if (node->link_valid > node->valid)
		node->valid = node->link_valid;
	cache_clean();
	//pthread_mutex_unlock(&cache.lock);
}

static int cache_get_attr(const char* path, struct stat* stbuf)
{
	struct node* node;
	int err = -EAGAIN;
	//pthread_mutex_lock(&cache.lock);
	node = cache_lookup(path);
	if (node != NULL) {
		time_t now = time(NULL);
		if (node->stat_valid - now >= 0) {
			*stbuf = node->stat;
			err = 0;
		}
	}
	//pthread_mutex_unlock(&cache.lock);
	return err;
}

uint64_t cache_get_write_ctr(void)
{
	uint64_t res;

	//pthread_mutex_lock(&cache.lock);
	res = cache.write_ctr;
	//pthread_mutex_unlock(&cache.lock);

	return res;
}

//static void* cache_init(struct fuse_conn_info* conn,
//	struct fuse_config* cfg)
//{
//	void* res;
//	res = cache.next_oper->init(conn, cfg);
//
//	// Cache requires a path for each request
//	cfg->nullpath_ok = 0;
//
//	return res;
//}

static int cache_getattr(const char* path, struct stat* stbuf,
	struct fuse_file_info* fi)
{
	int err = cache_get_attr(path, stbuf);
	if (err) {
		uint64_t wrctr = cache_get_write_ctr();
		err = cache.next_oper->getattr(path, stbuf, fi);
		if (!err)
			cache_add_attr(path, stbuf, wrctr);
	}
	return err;
}

static int cache_readlink(const char* path, char* buf, size_t size)
{
	struct node* node;
	int err;

	//pthread_mutex_lock(&cache.lock);
	node = cache_lookup(path);
	if (node != NULL) {
		time_t now = time(NULL);
		if (node->link_valid - now >= 0) {
			strncpy(buf, node->link, size - 1);
			buf[size - 1] = '\0';
			//pthread_mutex_unlock(&cache.lock);
			return 0;
		}
	}
	//pthread_mutex_unlock(&cache.lock);
	err = cache.next_oper->readlink(path, buf, size);
	if (!err)
		cache_add_link(path, buf, size);

	return err;
}


static int cache_opendir(const char* path, struct fuse_file_info* fi)
{
	(void)path;
	struct file_handle* cfi;

	cfi = malloc(sizeof(struct file_handle));
	if (cfi == NULL)
		return -ENOMEM;
	cfi->is_open = 0;
	fi->fh = (unsigned long)cfi;
	return 0;
}

static int cache_releasedir(const char* path, struct fuse_file_info* fi)
{
	int err;
	struct file_handle* cfi;

	cfi = (struct file_handle*)fi->fh;

	if (cfi->is_open) {
		fi->fh = cfi->fs_fh;
		err = cache.next_oper->releasedir(path, fi);
	}
	else
		err = 0;

	free(cfi);
	return err;
}

static int cache_dirfill(void* buf, const char* name,
	const struct stat* stbuf, off_t off,
	enum fuse_fill_dir_flags flags)
{
	int err;
	struct readdir_handle* ch;

	ch = (struct readdir_handle*)buf;
	err = ch->filler(ch->buf, name, stbuf, off, flags);
	if (!err) {
		//g_ptr_array_add(ch->dir, g_strdup(name));
		if (stbuf->st_mode & S_IFMT) {
			char* fullpath;
			const char* basepath = !ch->path[1] ? "" : ch->path;

			//fullpath = g_strdup_printf("%s/%s", basepath, name);
			cache_add_attr(fullpath, stbuf, ch->wrctr);
			//g_free(fullpath);
		}
	}
	return err;
}

static int cache_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
	off_t offset, struct fuse_file_info* fi,
	enum fuse_readdir_flags flags)
{
	struct readdir_handle ch;
	struct file_handle* cfi;
	int err;
	char** dir;
	struct node* node;

	//assert(offset == 0);

	//(&cache.lock);
	node = cache_lookup(path);
	if (node != NULL && node->dir != NULL) {
		time_t now = time(NULL);
		if (node->dir_valid - now >= 0) {
			for (dir = node->dir; *dir != NULL; dir++)
				// FIXME: What about st_mode?
				filler(buf, *dir, NULL, 0, 0);
			//pthread_mutex_unlock(&cache.lock);
			return 0;
		}
	}
	//pthread_mutex_unlock(&cache.lock);

	cfi = (struct file_handle*)fi->fh;
	if (cfi->is_open)
		fi->fh = cfi->fs_fh;
	else {
		if (cache.next_oper->opendir) {
			err = cache.next_oper->opendir(path, fi);
			if (err)
				return err;
		}
		cfi->is_open = 1;
		cfi->fs_fh = fi->fh;
	}

	ch.path = path;
	ch.buf = buf;
	ch.filler = filler;
	//ch.dir = g_ptr_array_new();
	ch.wrctr = cache_get_write_ctr();
	err = cache.next_oper->readdir(path, &ch, cache_dirfill, offset, fi, flags);
	//g_ptr_array_add(ch.dir, NULL);
	//dir = (char**)ch.dir->pdata;
	if (!err) {
		cache_add_dir(path, dir);
	}
	else {
		//g_strfreev(dir);
	}
	//g_ptr_array_free(ch.dir, FALSE);

	return err;
}

//static int cache_mknod(const char* path, mode_t mode, dev_t rdev)
//{
//	int err = cache.next_oper->mknod(path, mode, rdev);
//	if (!err)
//		cache_invalidate_dir(path);
//	return err;
//}
//
//static int cache_mkdir(const char* path, mode_t mode)
//{
//	int err = cache.next_oper->mkdir(path, mode);
//	if (!err)
//		cache_invalidate_dir(path);
//	return err;
//}
//
//static int cache_unlink(const char* path)
//{
//	int err = cache.next_oper->unlink(path);
//	if (!err)
//		cache_invalidate_dir(path);
//	return err;
//}
//
//static int cache_rmdir(const char* path)
//{
//	int err = cache.next_oper->rmdir(path);
//	if (!err)
//		cache_invalidate_dir(path);
//	return err;
//}
//
//static int cache_symlink(const char* from, const char* to)
//{
//	int err = cache.next_oper->symlink(from, to);
//	if (!err)
//		cache_invalidate_dir(to);
//	return err;
//}
//
//static int cache_rename(const char* from, const char* to, unsigned int flags)
//{
//	int err = cache.next_oper->rename(from, to, flags);
//	if (!err)
//		cache_do_rename(from, to);
//	return err;
//}
//
//static int cache_link(const char* from, const char* to)
//{
//	int err = cache.next_oper->link(from, to);
//	if (!err) {
//		cache_invalidate(from);
//		cache_invalidate_dir(to);
//	}
//	return err;
//}
//
//static int cache_chmod(const char* path, mode_t mode,
//	struct fuse_file_info* fi)
//{
//	int err = cache.next_oper->chmod(path, mode, fi);
//	if (!err)
//		cache_invalidate(path);
//	return err;
//}
//
//static int cache_chown(const char* path, uid_t uid, gid_t gid,
//	struct fuse_file_info* fi)
//{
//	int err = cache.next_oper->chown(path, uid, gid, fi);
//	if (!err)
//		cache_invalidate(path);
//	return err;
//}
//
//static int cache_utimens(const char* path, const struct timespec tv[2],
//	struct fuse_file_info* fi)
//{
//	int err = cache.next_oper->utimens(path, tv, fi);
//	if (!err)
//		cache_invalidate(path);
//	return err;
//}
//
//static int cache_write(const char* path, const char* buf, size_t size,
//	off_t offset, struct fuse_file_info* fi)
//{
//	int res = cache.next_oper->write(path, buf, size, offset, fi);
//	if (res >= 0)
//		cache_invalidate_write(path);
//	return res;
//}
//
//static int cache_create(const char* path, mode_t mode,
//	struct fuse_file_info* fi)
//{
//	int err = cache.next_oper->create(path, mode, fi);
//	if (!err)
//		cache_invalidate_dir(path);
//	return err;
//}
//
//static int cache_truncate(const char* path, off_t size,
//	struct fuse_file_info* fi)
//{
//	int err = cache.next_oper->truncate(path, size, fi);
//	if (!err)
//		cache_invalidate(path);
//	return err;
//}
