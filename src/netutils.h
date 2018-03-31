/*
 * netutils.h - Network utilities
 *
 * Copyright (C) 2013 - 2016, Max Lv <max.c.lv@gmail.com>
 *
 * This file is part of the shadowsocks-libev.
 *
 * shadowsocks-libev is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * shadowsocks-libev is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with shadowsocks-libev; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef _NETUTILS_H
#define _NETUTILS_H

#ifndef __MINGW32__
#include <netdb.h>
#include <netinet/tcp.h>
#endif

/* Hard coded defines for TCP fast open on Android */
#ifdef __ANDROID__
#ifndef TCP_FASTOPEN
#define TCP_FASTOPEN   23
#endif
#ifndef MSG_FASTOPEN
#define MSG_FASTOPEN   0x20000000
#endif
#ifdef TCP_FASTOPEN_CONNECT
#undef TCP_FASTOPEN_CONNECT
#endif
#endif

/* Hard coded defines for TCP fast open on macOS */
#ifdef __APPLE__
#ifndef CONNECT_RESUME_ON_READ_WRITE
#define CONNECT_RESUME_ON_READ_WRITE    0x1 /* resume connect() on read/write */
#endif

#ifndef SAE_ASSOCID_ANY
#define SAE_ASSOCID_ANY 0
typedef __uint32_t sae_associd_t;
typedef __uint32_t sae_connid_t;
#endif

#ifndef CONNECT_DATA_IDEMPOTENT
#define CONNECT_DATA_IDEMPOTENT     0x2 /* data is idempotent */

/* sockaddr endpoints */
typedef struct sa_endpoints {
    unsigned int        sae_srcif;  /* optional source interface */
    const struct sockaddr   *sae_srcaddr;   /* optional source address */
    socklen_t       sae_srcaddrlen; /* size of source address */
    const struct sockaddr   *sae_dstaddr;   /* destination address */
    socklen_t       sae_dstaddrlen; /* size of destination address */
} sa_endpoints_t;

int connectx(int, const sa_endpoints_t *, sae_associd_t, unsigned int,
    const struct iovec *, unsigned int, size_t *, sae_connid_t *);
#endif
#endif

/* Backward compatibility for MPTCP_ENABLED between kernel 3 & 4 */
#ifndef MPTCP_ENABLED
#ifdef TCP_CC_INFO
#define MPTCP_ENABLED 42
#else
#define MPTCP_ENABLED 26
#endif
#endif

/** byte size of ip4 address */
#define INET_SIZE 4
/** byte size of ip6 address */
#define INET6_SIZE 16

size_t get_sockaddr_len(struct sockaddr *addr);
ssize_t get_sockaddr(char *host, char *port,
                     struct sockaddr_storage *storage, int block,
                     int ipv6first);
int set_reuseport(int socket);

#ifdef SET_INTERFACE
int setinterface(int socket_fd, const char *interface_name);
#endif

int bind_to_address(int socket_fd, const char *address);

/**
 * Compare two sockaddrs. Imposes an ordering on the addresses.
 * Compares address and port.
 * @param addr1: address 1.
 * @param addr2: address 2.
 * @param len: lengths of addr.
 * @return: 0 if addr1 == addr2. -1 if addr1 is smaller, +1 if larger.
 */
int sockaddr_cmp(struct sockaddr_storage *addr1,
                 struct sockaddr_storage *addr2, socklen_t len);

/**
 * Compare two sockaddrs. Compares address, not the port.
 * @param addr1: address 1.
 * @param addr2: address 2.
 * @param len: lengths of addr.
 * @return: 0 if addr1 == addr2. -1 if addr1 is smaller, +1 if larger.
 */
int sockaddr_cmp_addr(struct sockaddr_storage *addr1,
                      struct sockaddr_storage *addr2, socklen_t len);

int validate_hostname(const char *hostname, const int hostname_len);

#endif
