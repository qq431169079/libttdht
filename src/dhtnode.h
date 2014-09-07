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
/* @CFILE dhtnode.h
*
*  Author: Alf <naihe2010@126.com>
*/
/* @date Created: 2009/08/19 11:16:40 Alf*/

#ifndef _DHT_NODE_H_
#define _DHT_NODE_H_

#include "dhtbucket.h"

#include "queue.h"

#include <time.h>
#ifdef _WIN32
#include <WinSock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

#define DN_MAX_FAILED   5

#define DN_AGE(dn)              (time (NULL) - (dn)->m_lastseen)
#define DN_IS_GOOD(dn)          ((dn)->m_active)
#define DN_IS_BAD(dn)           ((dn)->m_inactive >= DN_MAX_FAILED)
#define DN_IS_QUESTIONABLE(dn)  (!(dn)->m_active)
#define DN_IS_ACTIVE(dn)        ((dn)->m_lastseen)
#define DN_IS_IN_RANGE(dn, b)   (db_is_inrange ((b), (dn)->hashsg))

#define DN_SET_GOOD(dn) do {                                      \
  if ((dn)->m_bucket != NULL && !DN_IS_GOOD(dn))                  \
    DB_NODE_NOW_GOOD((dn)->m_bucket, DN_IS_BAD(dn));              \
  (dn)->m_lastseen = time (NULL); (dn)->m_inactive = 0; (dn)->m_active = 1;     \
} while (0)

#define DN_SET_BAD(dn) do {                                  \
  if ((dn)->m_bucket != NULL && !DN_IS_BAD (dn))                  \
    DB_NODE_NOW_BAD((dn)->m_bucket, DN_IS_GOOD(dn));              \
  (dn)->m_inactive = DN_MAX_FAILED; (dn)->m_active = 0;             \
} while (0)

#define DN_INACTIVE(dn) do {                                 \
  if ((dn)->m_inactive + 1 == DN_MAX_FAILED) DN_SET_BAD(dn);   \
  else (dn)->m_inactive ++;                                       \
} while (0)

#define DN_UPDATE(dn) do {                                      \
    (dn)->m_active = DN_AGE (dn) < 15 * 60;                    \
} while (0)

#define DN_QUERIED(dn) do {                                     \
    if ((dn)->m_lastseen) DN_SET_GOOD(dn);                        \
} while (0)

#define DN_REPLIED(dn) DN_SET_GOOD(dn)

#define dn_cleanup(dn)  free (dn)

struct dht_node
{
  char hashsg[HASH_STRING_LEN + 1];

  struct sockaddr_in m_sockaddr;

  time_t m_lastseen;
  int m_active;
  int m_inactive;

  struct dht_bucket *m_bucket;

    LIST_ENTRY (dht_node) entries;
};

struct dht_node *dn_init (const char *, const struct sockaddr_in *);

struct dht_node *dn_init_object (const char *, struct dht_object *);

char *dn_store_compact (struct dht_node *, char *);

struct dht_object *dn_store_cache (struct dht_node *, struct dht_object *);

#endif
