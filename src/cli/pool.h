// src/cli/pool.h
#pragma once
#include "config.h"

#define GD_POOL_MIN 1
#define GD_POOL_MAX 16

typedef struct {
	GDSSH* conn[GD_POOL_MAX];
	int size;
	volatile long rr;		/* round-robin counter */
} GDPOOL;

extern GDPOOL g_pool;

/* Build `size` SSH connections (clamped to [GD_POOL_MIN, GD_POOL_MAX]).
 * Returns the number actually established (0 on total failure). */
int gd_pool_init(int size);

/* Gracefully tear down every pooled connection. */
void gd_pool_free(void);

/* Round-robin a connection for new/stateless work. */
GDSSH* gd_pool_pick(void);

/* Index of c within the pool, or -1 if it is not pooled. For logging: a
 * reconnect line is far easier to read when it names which connection moved. */
int gd_pool_index(const GDSSH* c);
