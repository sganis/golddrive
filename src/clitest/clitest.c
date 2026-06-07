// src/clitest/clitest.c
// Native unit tests for the pure helpers in src/cli/parse.c and the string
// helpers in src/cli/util.c. Hermetic: no WinFsp, no libssh2.lib link.
// Build & run: tools\build_clitest.bat
#pragma warning(push, 3)
#include <libssh2.h>
#pragma warning(pop)
#include "../cli/parse.h"
#include "../cli/util.h"
#include <stdio.h>
#include <string.h>

static int g_checks = 0;
static int g_failures = 0;

int run_fuzz(void);     /* src/clitest/fuzz.c */
int run_net_tests(void); /* src/clitest/nettest.c */

#define CHECK(cond, msg) do {                              \
	g_checks++;                                            \
	if (!(cond)) {                                         \
		g_failures++;                                      \
		printf("  FAIL: %s  (%s:%d)\n", msg, __FILE__, __LINE__); \
	}                                                      \
} while (0)

static void test_knownhost_keytype(void)
{
	printf("gd_knownhost_keytype...\n");
	CHECK(gd_knownhost_keytype(LIBSSH2_HOSTKEY_TYPE_RSA)       == LIBSSH2_KNOWNHOST_KEY_SSHRSA,    "RSA -> SSHRSA");
	CHECK(gd_knownhost_keytype(LIBSSH2_HOSTKEY_TYPE_DSS)       == LIBSSH2_KNOWNHOST_KEY_SSHDSS,    "DSS -> SSHDSS");
	CHECK(gd_knownhost_keytype(LIBSSH2_HOSTKEY_TYPE_ECDSA_256) == LIBSSH2_KNOWNHOST_KEY_ECDSA_256, "ECDSA256");
	CHECK(gd_knownhost_keytype(LIBSSH2_HOSTKEY_TYPE_ECDSA_384) == LIBSSH2_KNOWNHOST_KEY_ECDSA_384, "ECDSA384");
	CHECK(gd_knownhost_keytype(LIBSSH2_HOSTKEY_TYPE_ECDSA_521) == LIBSSH2_KNOWNHOST_KEY_ECDSA_521, "ECDSA521");
	CHECK(gd_knownhost_keytype(LIBSSH2_HOSTKEY_TYPE_ED25519)   == LIBSSH2_KNOWNHOST_KEY_ED25519,   "ED25519");
	/* the H1 bug: ed25519/ecdsa must NOT collapse to the SSHDSS bucket */
	CHECK(gd_knownhost_keytype(LIBSSH2_HOSTKEY_TYPE_ED25519)   != LIBSSH2_KNOWNHOST_KEY_SSHDSS,    "ed25519 != dss (regression)");
	CHECK(gd_knownhost_keytype(LIBSSH2_HOSTKEY_TYPE_ECDSA_256) != LIBSSH2_KNOWNHOST_KEY_SSHDSS,    "ecdsa != dss (regression)");
	CHECK(gd_knownhost_keytype(LIBSSH2_HOSTKEY_TYPE_UNKNOWN)   == 0, "unknown -> 0");
	CHECK(gd_knownhost_keytype(999)                            == 0, "garbage -> 0");
}

static void test_normalize_link_path(void)
{
	printf("normalize_link_path...\n");
	char b[64];
	int n;

	n = normalize_link_path("/home/user", b, sizeof b);
	CHECK(n == 10 && strcmp(b, "/home/user") == 0, "plain path");
	n = normalize_link_path("/home//user", b, sizeof b);
	CHECK(strcmp(b, "/home/user") == 0, "collapse double slash");
	n = normalize_link_path("/a///b////c", b, sizeof b);
	CHECK(strcmp(b, "/a/b/c") == 0, "collapse many slashes");
	n = normalize_link_path("/home/user/", b, sizeof b);
	CHECK(strcmp(b, "/home/user") == 0, "strip trailing slash");
	n = normalize_link_path("/", b, sizeof b);
	CHECK(n == 1 && strcmp(b, "/") == 0, "bare root preserved");
	n = normalize_link_path("//", b, sizeof b);
	CHECK(strcmp(b, "/") == 0, "double root -> root");
	n = normalize_link_path("", b, sizeof b);
	CHECK(n == 0 && b[0] == '\0', "empty input");
	n = normalize_link_path("/aaaaaaaaaa", b, 5);
	CHECK(n == -1, "overflow guarded");
	n = normalize_link_path("/x", NULL, 10);
	CHECK(n == -1, "null out guarded");
}

static void test_extract_rcode(void)
{
	printf("extract_rcode...\n");
	int rc;
	int off;

	off = extract_rcode("hello\nRCODE=0\n", &rc);
	CHECK(off == 6 && rc == 0, "basic, code 0");
	off = extract_rcode("RCODE=42\n", &rc);
	CHECK(off == 0 && rc == 42, "code 42 at start");
	off = extract_rcode("RCODE=255\n", &rc);
	CHECK(rc == 255, "code 255");
	/* hardening: a fake sentinel in the payload must not win over the real one */
	off = extract_rcode("see RCODE= in output\nRCODE=7\n", &rc);
	CHECK(rc == 7, "last sentinel wins");
	off = extract_rcode("no sentinel here", &rc);
	CHECK(off == -1 && rc == 0, "missing sentinel -> -1");
	off = extract_rcode(NULL, &rc);
	CHECK(off == -1 && rc == 0, "null input safe");
}

static void test_str_replace(void)
{
	printf("str_replace (F3)...\n");
	char out[64];

	str_replace("a:b", ":", "", out, sizeof out);
	CHECK(strcmp(out, "ab") == 0, "remove colon");
	str_replace("hello", "l", "L", out, sizeof out);
	CHECK(strcmp(out, "heLLo") == 0, "same-length replace");
	str_replace("aaa", "a", "XY", out, sizeof out);
	CHECK(strcmp(out, "XYXYXY") == 0, "growing replace");
	str_replace("abc", "z", "Q", out, sizeof out);
	CHECK(strcmp(out, "abc") == 0, "no match -> unchanged");

	/* bounds regression (audit #3): a too-small result must not overflow.
	 * Canary bytes after the writable window must stay intact. */
	char z[16];
	memset(z, 0xAA, sizeof z);
	str_replace("aaaa", "a", "XY", z, 4);   /* window = z[0..3] */
	int canary_ok = 1;
	for (int i = 4; i < 16; i++)
		if ((unsigned char)z[i] != 0xAA) canary_ok = 0;
	CHECK(canary_ok, "no write past result_size");
	CHECK(strlen(z) <= 3, "result fits buffer");
}

static void test_str_helpers(void)
{
	printf("str helpers...\n");
	CHECK(str_contains("hello", "ell") == 1, "contains hit");
	CHECK(str_contains("hello", "xyz") == 0, "contains miss");
	CHECK(str_startswith("hello", "he") == 1, "startswith hit");
	CHECK(str_startswith("he", "hello") == 0, "startswith too short");
	CHECK(hash_path("/a/b") == hash_path("/a/b"), "hash deterministic");
	CHECK(hash_path("/a/b") != hash_path("/a/c"), "hash distinguishes");

	char* d = str_ndup("hello", 3);
	CHECK(d != NULL && strcmp(d, "hel") == 0, "str_ndup");
	free(d);

	char s[] = "hi  ";
	str_trim(s);
	CHECK(strcmp(s, "hi") == 0, "str_trim trailing");
}

static void test_parse_remote(void)
{
	printf("parse_remote_str (F1)...\n");
	gd_remote r;

	CHECK(parse_remote_str("//golddrive/user@host", &r) == 0, "basic parses");
	CHECK(strcmp(r.service, "golddrive") == 0, "service");
	CHECK(strcmp(r.user, "user") == 0 && r.has_user, "user");
	CHECK(strcmp(r.host, "host") == 0, "host");
	CHECK(!r.has_port && r.port == 0, "no port");
	CHECK(!r.has_locuser, "no locuser");
	CHECK(strcmp(r.root, "/") == 0 && r.has_root == 0, "default root");
	CHECK(strcmp(r.mountpoint, "user@host") == 0, "mountpoint is raw instance");

	CHECK(parse_remote_str("//golddrive/locuser=user@host!2222/data", &r) == 0, "full parses");
	CHECK(strcmp(r.locuser, "locuser") == 0 && r.has_locuser, "locuser");
	CHECK(strcmp(r.user, "user") == 0, "full user");
	CHECK(strcmp(r.host, "host") == 0, "full host");
	CHECK(r.has_port && r.port == 2222, "port 2222");
	CHECK(strcmp(r.root, "/data") == 0 && r.has_root == 1, "root /data");

	/* backslashes are normalized to '/' */
	CHECK(parse_remote_str("\\\\golddrive\\user@host", &r) == 0, "backslash parses");
	CHECK(strcmp(r.service, "golddrive") == 0 && strcmp(r.host, "host") == 0, "backslash normalized");

	/* host only, no user */
	CHECK(parse_remote_str("//golddrive/host", &r) == 0, "host only parses");
	CHECK(strcmp(r.host, "host") == 0 && !r.has_user, "host only, no user");

	/* nested path */
	CHECK(parse_remote_str("//golddrive/user@host/sub/dir", &r) == 0, "nested parses");
	CHECK(strcmp(r.root, "/sub/dir") == 0 && r.has_root == 1, "nested root");

	CHECK(parse_remote_str("", &r) == -1, "empty rejected");
	CHECK(parse_remote_str(NULL, &r) == -1, "null rejected");
}

static void test_parse_json(void)
{
	printf("parse_json_buffer (F2)...\n");
	gd_json g;
	const char* cfg =
		"{\"LogFile\":\"/tmp/gd.log\",\"UsageUrl\":\"https://x/u\","
		"\"Drives\":{"
		"\"Z:\":{\"AppKey\":\"/home/u/.ssh/id\",\"Args\":\"-o foo\"},"
		"\"Y:\":{\"AppKey\":\"/other/key\"}"
		"}}";

	CHECK(parse_json_buffer(cfg, "Z:", &g) == 0, "Z: parses");
	CHECK(g.has_logfile && strcmp(g.logfile, "/tmp/gd.log") == 0, "logfile");
	CHECK(g.has_usageurl && strcmp(g.usageurl, "https://x/u") == 0, "usageurl");
	CHECK(g.has_pkey && strcmp(g.pkey, "/home/u/.ssh/id") == 0, "Z: appkey");
	CHECK(g.has_args && strcmp(g.args, "-o foo") == 0, "Z: args");

	CHECK(parse_json_buffer(cfg, "Y:", &g) == 0, "Y: parses");
	CHECK(g.has_pkey && strcmp(g.pkey, "/other/key") == 0, "Y: appkey");
	CHECK(!g.has_args, "Y: no args");
	CHECK(g.has_logfile, "Y: still sees top-level logfile");

	CHECK(parse_json_buffer(cfg, "Q:", &g) == 0, "missing drive ok");
	CHECK(!g.has_pkey && !g.has_args, "missing drive -> no per-drive fields");
	CHECK(g.has_logfile && g.has_usageurl, "missing drive -> top-level still set");

	CHECK(parse_json_buffer("{}", "Z:", &g) == 0, "empty object ok");
	CHECK(!g.has_logfile && !g.has_pkey, "empty -> nothing set");

	CHECK(parse_json_buffer("not json", "Z:", &g) == -1, "malformed rejected");
	CHECK(parse_json_buffer(NULL, "Z:", &g) == -1, "null json rejected");
}

static void test_pool_math(void)
{
	printf("pool math (I1: clamp_int / rr_index)...\n");

	CHECK(clamp_int(5, 1, 16) == 5, "in range");
	CHECK(clamp_int(0, 1, 16) == 1, "below -> lo");
	CHECK(clamp_int(-3, 1, 16) == 1, "negative -> lo");
	CHECK(clamp_int(20, 1, 16) == 16, "above -> hi");
	CHECK(clamp_int(1, 1, 16) == 1 && clamp_int(16, 1, 16) == 16, "bounds inclusive");

	CHECK(rr_index(1, 4) == 0, "rr first slot");
	CHECK(rr_index(4, 4) == 3, "rr last slot");
	CHECK(rr_index(5, 4) == 0, "rr wraps");
	CHECK(rr_index(8, 4) == 3, "rr wraps full");
	CHECK(rr_index(1, 1) == 0 && rr_index(100, 1) == 0, "single slot");
	CHECK(rr_index(3, 0) == 0, "size 0 guarded");

	/* even distribution: counters 1..8 over 4 slots hit each slot twice */
	int hits[4] = { 0, 0, 0, 0 };
	for (long c = 1; c <= 8; c++)
		hits[rr_index(c, 4)]++;
	CHECK(hits[0] == 2 && hits[1] == 2 && hits[2] == 2 && hits[3] == 2, "even round-robin");
}

int main(void)
{
	printf("== golddrive native unit tests ==\n");
	test_knownhost_keytype();
	test_normalize_link_path();
	test_extract_rcode();
	test_str_replace();
	test_str_helpers();
	test_parse_remote();
	test_parse_json();
	test_pool_math();
	g_failures += run_net_tests();
	g_failures += run_fuzz();

	printf("\n%d checks, %d failures\n", g_checks, g_failures);
	if (g_failures) {
		printf("RESULT: FAIL\n");
		return 1;
	}
	printf("RESULT: PASS\n");
	return 0;
}
