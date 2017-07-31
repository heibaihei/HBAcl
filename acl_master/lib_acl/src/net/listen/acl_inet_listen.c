#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef	ACL_UNIX
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_sys_patch.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_iostuff.h"
#include "net/acl_host_port.h"
#include "net/acl_sane_socket.h"
#include "net/acl_listen.h"

#endif

static ACL_SOCKET inet_listen(const char *addr, const struct addrinfo *res,
	int backlog, int blocking)
{
	ACL_SOCKET sock = acl_sane_bind(res);
#if defined(TCP_FASTOPEN) && defined(USE_FASTOPEN)
	int        on;
#endif

	if (sock == ACL_SOCKET_INVALID)
		return ACL_SOCKET_INVALID;

#if defined(TCP_FASTOPEN) && defined(USE_FASTOPEN)
	on = 1;
	if (setsockopt(sock, IPPROTO_TCP, TCP_FASTOPEN,
		(const void *) &on, sizeof(on)) < 0)
	{
		acl_msg_warn("%s: setsocket(TCP_FASTOPEN): %s",
			__FUNCTION__, acl_last_serror());
	}
#endif

	acl_non_blocking(sock, blocking);

	if (listen(sock, backlog) < 0) {
		acl_socket_close(sock);
		acl_msg_error("%s: listen error: %s, addr=%s",
			__FUNCTION__, acl_last_serror(), addr);
		return ACL_SOCKET_INVALID;
	}

	acl_msg_info("%s: listen %s ok", __FUNCTION__, addr);
	return sock;
}

/* acl_inet_listen - create TCP listener */

ACL_SOCKET acl_inet_listen(const char *addr, int backlog, int blocking)
{
	struct addrinfo *res0 = acl_host_addrinfo(addr, SOCK_STREAM), *res;
	ACL_SOCKET sock;

	if (res0 == NULL)
		return ACL_SOCKET_INVALID;

	sock = ACL_SOCKET_INVALID;

	for (res = res0; res != NULL; res = res->ai_next) {
		sock = inet_listen(addr, res, backlog, blocking);
		if (sock != ACL_SOCKET_INVALID)
			break;
	}

	freeaddrinfo(res0);
	return sock;
}

ACL_SOCKET acl_inet_accept(ACL_SOCKET listen_fd)
{
	return acl_inet_accept_ex(listen_fd, NULL, 0);
}

ACL_SOCKET acl_inet_accept_ex(ACL_SOCKET listen_fd, char *ipbuf, size_t size)
{
	struct sockaddr_storage sa;
	socklen_t len = sizeof(sa);
	ACL_SOCKET fd;

	memset(&sa, 0, sizeof(sa));

	/* when client_addr not null and protocol is AF_INET, acl_sane_accept
	 * will set nodelay on the accepted socket, 2008.9.4, zsx
	 */
	fd = acl_sane_accept(listen_fd, (struct sockaddr *)&sa, &len);
	if (fd == ACL_SOCKET_INVALID)
		return fd;

	if (ipbuf != NULL && size > 0 && acl_getpeername(fd, ipbuf, size) < 0)
		ipbuf[0] = 0;

	return fd;
}
