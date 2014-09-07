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
/* @CFILE dhtrouter.h
*
*  Author: Alf <naihe2010@126.com>
*/
/* @date Created: 2009/08/19 11:16:40 Alf*/

#ifndef _DHT_ROUTER_H_
#define _DHT_ROUTER_H_

#include "dhtnode.h"
#include "dhtserver.h"

#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX	0x1000
#endif
#ifdef WIN32
#define ssize_t int
#endif

#define DR_SIZE_TOKEN               8
#define DR_TIMEOUT_BOOTSTRAP_RETRY  (60)
#define DR_TIMEOUT_UPDATE           (15 * 60)
#define DR_TIMEOUT_BUCKET_BOOTSTRAP (15 * 60)
#define DR_TIMEOUT_REMOVE_NODE      (4 * 60 * 60)
#define DR_TIMEOUT_PEER_ANNOUNCE    (30 * 60)

#define DR_NUM_BOOTSTRAP_COMPLETE   32
#define DR_NUM_BOOTSTRAP_CONTACTS   64

#define DR_IS_ACTIVE(dr)                   (dr->m_fdp)
#define DR_RESET_STATISTICS(dr)            (DS_RESET_STATISTICS(dr->m_server))

/* 
 * timer callback function
 * return 0 if the function should be removed, other wise keep
 * */
typedef int (*dht_source) (void *);

#define DHT_SOURCE(f)   (int (*) (void *)) f

struct timer
{
  struct timeval at[1];
  dht_source cb;
  void *arg;
    LIST_ENTRY (timer) entries;
};

struct dht_action
{
  /* 
   * 0 means nothing,
   * 1 means publish
   * 2 means search */
  int action;
  char actbuf[PATH_MAX];
  int actport;
  void (*actcb) (const char *, const char *, void *);
  void *actarg;
    LIST_ENTRY (dht_action) entries;
};

enum
{ DHT_ACTION_NONE = 0, DHT_ACTION_PUB, DHT_ACTION_SEARCH };

typedef struct
{
  void *fdp;
  ssize_t (*read) (void *, char *, size_t, struct sockaddr_in *, int);
  ssize_t (*write) (void *, const char *, size_t, struct sockaddr_in *);
} dhtio_t;

struct dht_router
{
  struct dht_node *node;

  int m_tasktimeout;

  struct dht_server *m_server;

  struct timer *boot_timer;

  struct map m_nodes;
  struct map m_buckets;
  struct map m_trackers;
  struct map m_contacts;

  unsigned int m_contacts_count;
  int m_numrefresh;
  int m_networkup;
  int m_curtoken;
  int m_prevtoken;

    LIST_HEAD (timer_list, timer) timer_list[1];
    LIST_HEAD (action_list, dht_action) action_list[1];

  void *m_fdp;
  ssize_t (*read) (void *, char *, size_t, struct sockaddr_in *, int);
  ssize_t (*write) (void *, const char *, size_t, struct sockaddr_in *);

  /* 
   * 0 means running, !0 means quit 
   * */
  int quit;
};

struct dht_router *dr_init (struct dht_object *, int, dhtio_t *);

void dr_cleanup (struct dht_router *);

int dr_start (struct dht_router *, int);

void dr_stop (struct dht_router *);

int dr_run (struct dht_router *);

void dr_pub (struct dht_router *, const char *, unsigned short);
void dr_get (struct dht_router *, const char *,
	     void (*)(const char *, const char *, void *), void *);
void dr_announce (struct dht_router *, const char *, void *);
void dr_cancel_announce (struct dht_router *, const char *,
			 struct dht_tracker *);

struct dht_tracker *dr_get_tracker (struct dht_router *, const char *, int);

int dr_want_node (struct dht_router *, const char *);
struct dht_node *dr_get_node (struct dht_router *, const char *);

void dr_add_contact (struct dht_router *, const char *, int);
void dr_contact (struct dht_router *, struct sockaddr_in *, int);
struct dht_node *dr_find_node (struct dht_router *,
			       const struct sockaddr_in *);

struct dht_node *dr_node_queried (struct dht_router *, const char *,
				  struct sockaddr_in *);
struct dht_node *dr_node_replied (struct dht_router *, const char *,
				  const struct sockaddr_in *);
struct dht_node *dr_node_inactive (struct dht_router *, const char *,
				   const struct sockaddr_in *);
void dr_node_invalid (struct dht_router *, const char *);

char *dr_store_closest_nodes (struct dht_router *, const char *, char *,
			      char *);
struct dht_object *dr_store_cache (struct dht_router *, struct dht_object *);

char *dr_generate_token (struct dht_router *, const struct sockaddr_in *,
			 int, char *);
char *dr_make_token (struct dht_router *, const struct sockaddr_in *, char *);
int dr_token_valid (struct dht_router *, const char *,
		    const struct sockaddr_in *);

struct map_node *dr_find_bucket (struct dht_router *, const char *);
int dr_add_node_to_bucket (struct dht_router *, struct dht_node *);
void dr_delete_node (struct dht_router *, struct map_node *);
struct map_node *dr_split_bucket (struct dht_router *, struct map_node *,
				  struct dht_node *);

void dr_bootstrap (struct dht_router *);
void dr_bootstrap_bucket (struct dht_router *, struct dht_bucket *);

struct timer *dr_timer_add (struct dht_router *, int, dht_source, void *);

void dr_timer_remove (struct dht_router *, struct timer *);

#endif
