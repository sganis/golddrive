// src/cli/parse.h
#pragma once
#include <stddef.h>

/* Pure, network-free helpers extracted from the SSH/FUSE code so they can be
 * unit-tested and fuzzed in isolation (see src/clitest). None of these touch
 * libssh2 sessions, sockets, or WinFsp; they operate purely on caller buffers. */

/* Map a libssh2 host-key type (LIBSSH2_HOSTKEY_TYPE_*) to the matching
 * known_hosts key-encoding bits (LIBSSH2_KNOWNHOST_KEY_*). Covers RSA, DSS,
 * ECDSA 256/384/521 and ED25519. Returns 0 for unknown/unsupported types. */
int gd_knownhost_keytype(int hostkey_type);

/* Normalize a symlink target: collapse runs of '/' into a single '/' and strip
 * one trailing '/' (unless the whole path is "/"). Writes a NUL-terminated
 * result into out (capacity out_size). Returns the result length, or -1 if out
 * is too small or any argument is invalid. */
int normalize_link_path(const char* in, char* out, size_t out_size);

/* Locate the "RCODE=<n>" sentinel appended to remote shell output and parse the
 * exit code. Scans for the LAST occurrence so payload text containing "RCODE="
 * cannot spoof the real trailing sentinel. On success returns the byte offset of
 * the sentinel (so the caller can truncate output there) and writes the code to
 * *rcode. Returns -1 and sets *rcode = 0 when no sentinel is present. */
int extract_rcode(const char* out, int* rcode);

/* Parsed components of a golddrive remote string. Optional fields carry a
 * has_* flag so the caller can tell "absent" from "present but empty". */
#define GD_REMOTE_MAX 1024
typedef struct {
	char service[256];
	char mountpoint[GD_REMOTE_MAX];
	char host[256];
	char locuser[256];
	char user[256];
	char root[GD_REMOTE_MAX];
	int  port;
	int  has_locuser;
	int  has_user;
	int  has_port;
	int  has_root;
} gd_remote;

/* Parse a remote of the form  [//]service/[locuser=]user@host[!port][/path]
 * (backslashes are accepted and normalized to '/'). Fills *out; missing
 * optional fields stay empty with their has_* flag clear. `root` is the path
 * with a leading '/' ensured. Returns 0 on success, -1 on NULL/empty/too-long
 * input. Pure: no allocation, no globals, no network. */
int parse_remote_str(const char* remote, gd_remote* out);

/* Parsed values from a golddrive config.json. has_* distinguishes "absent"
 * from "present but empty". */
typedef struct {
	char logfile[GD_REMOTE_MAX];
	char usageurl[GD_REMOTE_MAX];
	char pkey[GD_REMOTE_MAX];
	char args[GD_REMOTE_MAX];
	int  has_logfile;
	int  has_usageurl;
	int  has_pkey;
	int  has_args;
} gd_json;

/* Parse config JSON for the given drive key, filling top-level LogFile/UsageUrl
 * and the per-drive AppKey/Args into *out. Returns 0 on success (even when the
 * drive or some fields are absent), -1 on malformed JSON or bad args. Pure: no
 * file I/O, no globals. */
int parse_json_buffer(const char* json, const char* drive, gd_json* out);
