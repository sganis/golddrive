// src/cli/pool.c
#include "config.h"
#include "util.h"
#include "gd.h"
#include "parse.h"
#include "pool.h"
#include <string.h>

GDPOOL g_pool;

/* the active connection is per-thread: each WinFsp dispatcher thread locks a
 * pooled connection into its own g_ssh, so unrelated I/O runs in parallel */
__declspec(thread) GDSSH* g_ssh;

/* When set, gd_lock() reuses this connection instead of round-robining.
 * Recovery paths pin the connection they just healed so the retry actually runs
 * on it: round-robin would otherwise hand the retry a different connection,
 * which after a network drop is just as dead as the one that failed. Pinning is
 * thread-local and NULL outside a retry, so normal I/O is unaffected. */
static __declspec(thread) GDSSH* g_pin;

/* Generation of g_ssh at the moment this thread acquired its lock. Recovery
 * paths compare it against the connection's current generation to tell "still
 * broken" from "another thread already rebuilt this connection". */
__declspec(thread) long g_gen;

int gd_pool_init(int size)
{
	memset(&g_pool, 0, sizeof g_pool);
	size = clamp_int(size, GD_POOL_MIN, GD_POOL_MAX);

	int built = 0;
	for (int i = 0; i < size; i++) {
		GDSSH* c = gd_init_ssh();
		if (!c)
			break;
		g_pool.conn[i] = c;
		built++;
	}
	g_pool.size = built;
	g_pool.rr = 0;
	return built;
}

void gd_pool_free(void)
{
	for (int i = 0; i < g_pool.size; i++) {
		if (g_pool.conn[i]) {
			gd_conn_free(g_pool.conn[i]);
			g_pool.conn[i] = NULL;
		}
	}
	g_pool.size = 0;
}

GDSSH* gd_pool_pick(void)
{
	if (g_pool.size <= 1)
		return g_pool.conn[0];
	long n = InterlockedIncrement(&g_pool.rr);
	return g_pool.conn[rr_index(n, g_pool.size)];
}

int gd_pool_index(const GDSSH* c)
{
	for (int i = 0; i < g_pool.size; i++)
		if (g_pool.conn[i] == c)
			return i;
	return -1;
}

void gd_lock(void)
{
	GDSSH* c = g_pin ? g_pin : gd_pool_pick();
	g_ssh = c;
	AcquireSRWLockExclusive(&c->lock);
	g_gen = c->generation;
}

void gd_pin(GDSSH* c)
{
	g_pin = c;
}

void gd_unpin(void)
{
	g_pin = NULL;
}

void gd_lock_conn(GDSSH* c)
{
	g_ssh = c;
	AcquireSRWLockExclusive(&c->lock);
	g_gen = c->generation;
}

void gd_unlock(void)
{
	ReleaseSRWLockExclusive(&g_ssh->lock);
}
