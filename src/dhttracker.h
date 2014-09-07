/*
* This file is part of the libttdht package
* Copyright (C) <2008>  <Alf>
*
* Contact: Alf <naihe2010@126.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
*/
/* @CFILE dhttracker.h
*
*  Author: Alf <naihe2010@126.com>
*/
/* @date Created: 2009/08/19 11:16:40 Alf*/

#ifndef _DHT_TRACKER_H_
#define _DHT_TRACKER_H_

#include "queue.h"

#include <time.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

#define DTK_MAX_PEERS 32
#define DTK_MAX_SIZE  128

#define DTK_EMPTY(dtk) ((dtk)->m_peers_count == 0)
#define DTK_SIZE(dtk)  ((dtk)->m_peers_count)

struct dht_sockaddr
{
  struct sockaddr_in m_sa;
  time_t m_lastseen;
    LIST_ENTRY (dht_sockaddr) entries;
};

struct dht_tracker
{
  LIST_HEAD (peerlist, dht_sockaddr) m_peers;
  unsigned int m_peers_count;
    LIST_ENTRY (dht_tracker) entries;
};

struct dht_tracker *dt_init (void);

void dt_cleanup (struct dht_tracker *);

void dt_add_peer (struct dht_tracker *, unsigned long, unsigned short);

struct string *dt_get_peers (struct dht_tracker *, unsigned int);

void dt_prune (struct dht_tracker *, int);

#endif
