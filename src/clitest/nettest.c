// src/clitest/nettest.c
// Network tests for gd_tcp_connect (I2). Uses an ephemeral loopback listener,
// so it needs no SSH server and stays hermetic. Returns the failure count.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "../cli/net.h"
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>

static int nfail = 0;
#define NCHECK(cond, msg) do { if (!(cond)) { nfail++; printf("  FAIL: %s\n", msg); } } while (0)

/* bind+listen on an ephemeral loopback port of the given family;
 * returns the listening socket and writes the chosen port */
static SOCKET start_listener(int family, int* port)
{
	SOCKET ls = socket(family, SOCK_STREAM, 0);
	if (ls == INVALID_SOCKET)
		return INVALID_SOCKET;

	if (family == AF_INET) {
		struct sockaddr_in a;
		memset(&a, 0, sizeof a);
		a.sin_family = AF_INET;
		a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		if (bind(ls, (struct sockaddr*)&a, sizeof a) != 0) { closesocket(ls); return INVALID_SOCKET; }
		int al = sizeof a;
		getsockname(ls, (struct sockaddr*)&a, &al);
		*port = ntohs(a.sin_port);
	}
	else {
		struct sockaddr_in6 a;
		memset(&a, 0, sizeof a);
		a.sin6_family = AF_INET6;
		a.sin6_addr = in6addr_loopback;
		if (bind(ls, (struct sockaddr*)&a, sizeof a) != 0) { closesocket(ls); return INVALID_SOCKET; }
		int al = sizeof a;
		getsockname(ls, (struct sockaddr*)&a, &al);
		*port = ntohs(a.sin6_port);
	}

	if (listen(ls, 1) != 0) { closesocket(ls); return INVALID_SOCKET; }
	return ls;
}

int run_net_tests(void)
{
	printf("gd_tcp_connect (I2)...\n");
	nfail = 0;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("  FAIL: WSAStartup\n");
		return 1;
	}

	/* IPv4 loopback: a live listener accepts the connection */
	int port = 0;
	SOCKET ls = start_listener(AF_INET, &port);
	if (ls == INVALID_SOCKET) {
		NCHECK(0, "ipv4 listener setup");
	}
	else {
		SOCKET c = gd_tcp_connect("127.0.0.1", port);
		NCHECK(c != INVALID_SOCKET, "connect ipv4 loopback");
		if (c != INVALID_SOCKET) closesocket(c);
		closesocket(ls);

		/* nothing listening now -> connection refused */
		SOCKET c2 = gd_tcp_connect("127.0.0.1", port);
		NCHECK(c2 == INVALID_SOCKET, "refused -> INVALID_SOCKET");
		if (c2 != INVALID_SOCKET) closesocket(c2);
	}

	/* IPv6 loopback when available (the reason for the getaddrinfo switch) */
	int port6 = 0;
	SOCKET ls6 = start_listener(AF_INET6, &port6);
	if (ls6 == INVALID_SOCKET) {
		printf("  (ipv6 unavailable on this host, skipping ipv6 connect)\n");
	}
	else {
		SOCKET c = gd_tcp_connect("::1", port6);
		NCHECK(c != INVALID_SOCKET, "connect ipv6 loopback");
		if (c != INVALID_SOCKET) closesocket(c);
		closesocket(ls6);
	}

	WSACleanup();
	printf("  %d failures\n", nfail);
	return nfail;
}
