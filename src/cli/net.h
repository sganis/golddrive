// src/cli/net.h
#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>

/* Resolve host (DNS name, IPv4, or IPv6 literal) and open a connected TCP
 * socket to host:port, trying each resolved address in order. Returns a
 * connected blocking SOCKET, or INVALID_SOCKET on failure. The caller owns the
 * socket. WSAStartup must already have been called.
 *
 * Replaces the legacy gethostbyname path, which was IPv4-only and deprecated. */
SOCKET gd_tcp_connect(const char* host, int port);
