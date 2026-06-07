// src/clitest/fuzz.c
// Randomized property tests for the pure parsers. No libFuzzer/ASan needed:
// a deterministic PRNG drives many inputs and guard bytes around the output
// buffer catch out-of-bounds writes by hand. Returns the number of failures.
#include "../cli/parse.h"
#include <stdio.h>
#include <string.h>

/* deterministic xorshift32 — no time()/rand() so runs are reproducible */
static unsigned int s_state = 0x12345678u;
static unsigned int rnd(void)
{
	unsigned int x = s_state;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	s_state = x;
	return x;
}

#define GUARD 0xA5
#define CAP   32
#define ITERS 200000

int run_fuzz(void)
{
	printf("randomized property tests...\n");
	int failures = 0;

	/* normalize_link_path: never writes out of bounds; output has no "//",
	 * never grows, and the returned length matches strlen. */
	for (int it = 0; it < ITERS; it++) {
		char in[40];
		int len = (int)(rnd() % (sizeof(in) - 1));
		for (int i = 0; i < len; i++) {
			unsigned int r = rnd() % 5;
			in[i] = (r < 3) ? '/' : (char)('a' + (r - 3));
		}
		in[len] = '\0';

		unsigned char raw[CAP + 16];
		memset(raw, GUARD, sizeof raw);
		char* out = (char*)(raw + 8);
		int n = normalize_link_path(in, out, CAP);

		for (int g = 0; g < 8; g++) {
			if (raw[g] != GUARD || raw[8 + CAP + g] != GUARD) {
				printf("  FAIL: normalize_link_path OOB write, input=\"%s\"\n", in);
				failures++;
				break;
			}
		}
		if (n >= 0) {
			if ((int)strlen(out) != n) { printf("  FAIL: len mismatch \"%s\"\n", in); failures++; }
			if (strstr(out, "//"))     { printf("  FAIL: \"//\" remains \"%s\"\n", in); failures++; }
			if (n > len)               { printf("  FAIL: output grew \"%s\"\n", in); failures++; }
		}
	}

	/* extract_rcode: never crashes; a found offset is inside the string and a
	 * not-found result leaves *rcode at 0. */
	for (int it = 0; it < ITERS; it++) {
		char in[40];
		int len = (int)(rnd() % (sizeof(in) - 1));
		const char* pool = "RCODE=0123";
		for (int i = 0; i < len; i++)
			in[i] = pool[rnd() % 10];
		in[len] = '\0';

		int rc = -999;
		int off = extract_rcode(in, &rc);
		if (off >= 0 && off >= (int)strlen(in)) { printf("  FAIL: rcode offset OOB \"%s\"\n", in); failures++; }
		if (off < 0 && rc != 0)                 { printf("  FAIL: rcode not reset \"%s\"\n", in); failures++; }
	}

	printf("  %d iterations x2, %d failures\n", ITERS, failures);
	return failures;
}
