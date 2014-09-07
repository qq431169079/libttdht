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
/* @CFILE dhtserver.h
*
*  Author: Alf <naihe2010@126.com>
*/
/* @date Created: 2009/08/19 11:16:40 Alf*/

#ifndef _DHT_SERVER_H_
#define _DHT_SERVER_H_

#include "dhttrans.h"

#define DS_QUERIES_RECEIVED(ds)  (ds->m_queriesreceived)
#define DS_QUERIES_SENT(ds)      (ds->m_queriessent)
#define DS_REPLIES_RECEIVED(ds)  (ds->m_repliesreceived)

struct compact_node_info
{
  char id[HASH_STRING_LEN];
  char addr[4];
  char port[2];
};

struct dht_ttype_trans_t
{
  dht_trans_type key;
  struct dht_trans *trans;
    LIST_ENTRY (dht_ttype_trans_t) entries;
};

struct dht_server
{
  struct timer *timer;

  unsigned short port;

    LIST_HEAD (node_info_list, node_info) node_info_list;

  struct list m_searches;

  unsigned char *queries;

  struct dht_router *m_router;

  unsigned int m_queriesreceived;
  unsigned int m_queriessent;
  unsigned int m_repliesreceived;

    LIST_HEAD (trans_map, dht_ttype_trans_t) m_trans;

  int m_networkup;
};

struct dht_server *ds_init (struct dht_router *);

void ds_cleanup (struct dht_server *);

int ds_start (struct dht_server *, unsigned short);

void ds_stop (struct dht_server *);

void ds_restart (struct dht_server *);

void ds_ping (struct dht_server *, const char *, struct sockaddr_in *);

void ds_find_node (struct dht_server *, struct dht_bucket *, const char *);

void ds_announce (struct dht_server *, const char *, unsigned short, int,
		  void (*)(const char *, const char *, void *), void *);

void ds_cancel_announce (struct dht_server *, const char *, void *);

void ds_update (struct dht_server *);

int ds_process (struct dht_server *, struct sockaddr_in *, char *, int);

#endif
