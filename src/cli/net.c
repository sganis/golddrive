// src/cli/net.c
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "net.h"
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>

SOCKET gd_tcp_connect(const char* host, int port)
{
	if (!host)
		return INVALID_SOCKET;

	char portstr[16];
	snprintf(portstr, sizeof portstr, "%d", port);

	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;        /* IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	struct addrinfo* res = NULL;
	if (getaddrinfo(host, portstr, &hints, &res) != 0 || !res)
		return INVALID_SOCKET;

	SOCKET sock = INVALID_SOCKET;
	for (struct addrinfo* ai = res; ai; ai = ai->ai_next) {
		sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (sock == INVALID_SOCKET)
			continue;
		if (connect(sock, ai->ai_addr, (int)ai->ai_addrlen) == 0)
			break;                      /* connected */
		closesocket(sock);
		sock = INVALID_SOCKET;
	}

	freeaddrinfo(res);
	return sock;
}
