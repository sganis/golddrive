// src/cli/parse.c
#include "parse.h"
#pragma warning(push, 3)
#include <libssh2.h>
#pragma warning(pop)
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "jsmn.h"

int gd_knownhost_keytype(int hostkey_type)
{
	switch (hostkey_type) {
	case LIBSSH2_HOSTKEY_TYPE_RSA:       return LIBSSH2_KNOWNHOST_KEY_SSHRSA;
	case LIBSSH2_HOSTKEY_TYPE_DSS:       return LIBSSH2_KNOWNHOST_KEY_SSHDSS;
	case LIBSSH2_HOSTKEY_TYPE_ECDSA_256: return LIBSSH2_KNOWNHOST_KEY_ECDSA_256;
	case LIBSSH2_HOSTKEY_TYPE_ECDSA_384: return LIBSSH2_KNOWNHOST_KEY_ECDSA_384;
	case LIBSSH2_HOSTKEY_TYPE_ECDSA_521: return LIBSSH2_KNOWNHOST_KEY_ECDSA_521;
	case LIBSSH2_HOSTKEY_TYPE_ED25519:   return LIBSSH2_KNOWNHOST_KEY_ED25519;
	default:                             return 0;
	}
}

int normalize_link_path(const char* in, char* out, size_t out_size)
{
	if (!in || !out || out_size == 0)
		return -1;

	size_t n = 0;
	char prev = 0;
	for (const char* p = in; *p; p++) {
		if (*p == '/' && prev == '/')
			continue;            /* collapse consecutive slashes */
		if (n + 1 >= out_size)   /* keep room for the NUL */
			return -1;
		out[n++] = *p;
		prev = *p;
	}

	/* strip a single trailing slash, but keep a bare "/" intact */
	if (n > 1 && out[n - 1] == '/')
		n--;

	out[n] = '\0';
	return (int)n;
}

int extract_rcode(const char* out, int* rcode)
{
	if (rcode)
		*rcode = 0;
	if (!out)
		return -1;

	const char* sentinel = "RCODE=";
	const size_t slen = 6;

	/* take the LAST occurrence so embedded output can't spoof the sentinel */
	const char* last = NULL;
	for (const char* p = strstr(out, sentinel); p; p = strstr(p + slen, sentinel))
		last = p;
	if (!last)
		return -1;

	int val = 0;
	for (const char* d = last + slen; isdigit((unsigned char)*d); d++)
		val = val * 10 + (*d - '0');

	if (rcode)
		*rcode = val;
	return (int)(last - out);
}

static void copy_field(char* dst, size_t cap, const char* src)
{
	if (cap == 0)
		return;
	size_t i = 0;
	for (; src[i] && i + 1 < cap; i++)
		dst[i] = src[i];
	dst[i] = '\0';
}

int parse_remote_str(const char* remote, gd_remote* out)
{
	if (!remote || !out)
		return -1;
	memset(out, 0, sizeof(*out));

	/* private mutable copy — the parse tokenizes in place */
	char buf[GD_REMOTE_MAX];
	size_t rl = strlen(remote);
	if (rl == 0 || rl >= sizeof(buf))
		return -1;
	memcpy(buf, remote, rl + 1);

	/* backslash -> forward slash */
	for (char* q = buf; *q; q++)
		if (*q == '\\')
			*q = '/';

	/* skip leading slashes, then the service name up to the next slash */
	char* p = buf;
	while (*p == '/')
		p++;
	char* service = p;
	while (*p && *p != '/')
		p++;
	if (*p)
		*p++ = '\0';

	/* the remainder (instance + path) is the mountpoint, captured raw */
	copy_field(out->service, sizeof out->service, service);
	copy_field(out->mountpoint, sizeof out->mountpoint, p);

	/* instance: [locuser=]user@host[!port][/path] */
	char* host = p;
	char* locuser = NULL;
	char* user = NULL;
	char* port = NULL;
	while (*p && *p != '/') {
		if (*p == '=') { *p = '\0'; locuser = host; host = p + 1; }
		else if (*p == '@') { *p = '\0'; user = host; host = p + 1; }
		else if (*p == '!') { *p = '\0'; port = p + 1; }
		p++;
	}
	if (*p)
		*p++ = '\0';   /* terminate host/port token; p -> path */

	copy_field(out->host, sizeof out->host, host);
	if (locuser) {
		out->has_locuser = 1;
		copy_field(out->locuser, sizeof out->locuser, locuser);
	}
	if (user) {
		out->has_user = 1;
		copy_field(out->user, sizeof out->user, user);
	}
	if (port) {
		out->has_port = 1;
		out->port = atoi(port);
	}

	/* root: ensure a leading '/'. When the path already starts with '/',
	 * keep it verbatim and leave has_root clear (matches legacy behavior). */
	if (*p != '/') {
		out->root[0] = '/';
		copy_field(out->root + 1, sizeof(out->root) - 1, p);
		out->has_root = (int)(strlen(out->root) > 1);
	}
	else {
		copy_field(out->root, sizeof out->root, p);
		out->has_root = 0;
	}
	return 0;
}

static int json_eq(const char* json, const jsmntok_t* tok, const char* s)
{
	return (tok->type == JSMN_STRING
		&& (int)strlen(s) == tok->end - tok->start
		&& strncmp(json + tok->start, s, tok->end - tok->start) == 0) ? 0 : -1;
}

static void copy_token(char* dst, size_t cap, const char* json, const jsmntok_t* tok)
{
	int n = tok->end - tok->start;
	if (n < 0)
		n = 0;
	if ((size_t)n >= cap)
		n = (int)cap - 1;
	memcpy(dst, json + tok->start, (size_t)n);
	dst[n] = '\0';
}

int parse_json_buffer(const char* json, const char* drive, gd_json* out)
{
	if (!json || !drive || !out)
		return -1;
	memset(out, 0, sizeof(*out));

	int num_tokens = 1024;
	jsmntok_t* t = (jsmntok_t*)malloc((size_t)num_tokens * sizeof(jsmntok_t));
	if (!t)
		return -1;

	jsmn_parser p;
	jsmn_init(&p);
	int r = jsmn_parse(&p, json, strlen(json), t, (unsigned int)num_tokens);
	if (r < 1 || t[0].type != JSMN_OBJECT) {
		free(t);
		return -1;
	}

	/* loop over the keys of the root object (same index arithmetic as the
	 * original load_json token walk, relocated here verbatim) */
	for (int i = 1; i < r; i++) {
		if (i + 1 >= r) break;
		if (json_eq(json, &t[i], "LogFile") == 0) {
			copy_token(out->logfile, sizeof out->logfile, json, &t[i + 1]);
			out->has_logfile = 1;
			i++;
		}
		else if (json_eq(json, &t[i], "UsageUrl") == 0) {
			copy_token(out->usageurl, sizeof out->usageurl, json, &t[i + 1]);
			out->has_usageurl = 1;
			i++;
		}
		else if (json_eq(json, &t[i], "Drives") == 0) {
			if (i + 1 >= r) break;
			int ndrives = t[i + 1].size;
			i++;
			for (int j = 0; j < ndrives; j++) {
				if (i + 2 >= r) break;
				const jsmntok_t* keytok = &t[i + 1];
				int klen = keytok->end - keytok->start;
				const jsmntok_t* v = &t[i + 2];
				int match = (klen >= 0
					&& (int)strlen(drive) == klen
					&& strncmp(json + keytok->start, drive, (size_t)klen) == 0);

				if (match) {
					i = i + 2;
					for (int k = 0; k < v->size; k++) {
						if (i + 2 >= r) break;
						const jsmntok_t* kt = &t[i + 1];
						if (kt->type == JSMN_STRING) {
							if (json_eq(json, kt, "AppKey") == 0) {
								copy_token(out->pkey, sizeof out->pkey, json, &t[i + 2]);
								out->has_pkey = 1;
							}
							else if (json_eq(json, kt, "Args") == 0) {
								copy_token(out->args, sizeof out->args, json, &t[i + 2]);
								out->has_args = 1;
							}
							i = i + 2;
						}
						else if (kt->type == JSMN_ARRAY) {
							i = i + kt->size + 1;
						}
					}
				}
				else {
					/* skip this drive's value object. NOTE: the legacy
					 * load_json used "i + 3" here (vs "i + 2" in the match
					 * branch), an off-by-one that made AppKey/Args unreadable
					 * for any drive that wasn't first in config.json. */
					i = i + 2;
					for (int k = 0; k < v->size; k++) {
						if (i + 1 >= r) break;
						const jsmntok_t* kt = &t[i + 1];
						if (kt->type == JSMN_STRING)
							i = i + 2;
						else if (kt->type == JSMN_ARRAY)
							i = i + kt->size + 1;
					}
				}
			}
		}
		else {
			i++;
		}
	}

	free(t);
	return 0;
}
