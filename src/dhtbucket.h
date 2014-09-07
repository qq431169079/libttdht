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
/* @CFILE dhtbucket.h
*
*  Author: Alf <naihe2010@126.com>
*/
/* @date Created: 2009/08/19 11:16:40 Alf*/

#ifndef _DHT_BUCKET_H_
#define _DHT_BUCKET_H_

#include "dhtlib.h"
#include "queue.h"

#include <time.h>

#define DB_NUM_NODES        8

#define DB_IS_INRANGE(db, id)   ((hashsg_cmp ((id), (db)->m_begin)) >= 0 && (hashsg_cmp ((id), (db)->m_end)) <= 0)
#define DB_IS_FULL(db)          ((db)->m_size >= DB_NUM_NODES)
#define DB_IS_EMPTY(db)         ((db)->m_size <= 0)
#define DB_HAS_SPACE(db)        (!DB_IS_FULL(db) || (db)->m_bad > 0)
#define DB_AGE(db)              (time (NULL) - (db)->m_lastchanged)
#define DB_TOUCH(db)            (db)->m_lastchanged = time (NULL)
#define DB_UPDATE(db)           db_count (db)

#define DB_NODE_NOW_GOOD(db, was_bad) do {              \
  (db)->m_bad -= (was_bad);                             \
  (db)->m_good ++;                                      \
} while (0)

#define DB_NODE_NOW_BAD(db, was_good) do {              \
  (db)->m_good -= (was_good);                           \
  (db)->m_bad ++;                                       \
} while (0)

struct dht_bucket
{
  struct dht_bucket *m_parent;
  struct dht_bucket *m_child;

  time_t m_lastchanged;

  int m_good;
  int m_bad;

  char m_begin[HASH_STRING_LEN + 1];
  char m_end[HASH_STRING_LEN + 1];

  int m_size;
    LIST_HEAD (node_list, dht_node) m_nodes[1];

    LIST_ENTRY (dht_bucket) entries;
};

struct dht_bucket *db_init (const char *, const char *);

void db_cleanup (struct dht_bucket *db);

void db_add_node (struct dht_bucket *, struct dht_node *);

void db_count (struct dht_bucket *);

void db_remove_node (struct dht_bucket *, struct dht_node *);

void db_get_mid_point (struct dht_bucket *, char *);
void db_get_random_id (struct dht_bucket *, char *);

struct dht_bucket *db_split (struct dht_bucket *, const char *);

struct dht_node *db_find_replacement (struct dht_bucket *, int);

struct db_chain
{
  struct dht_bucket *m_restart;
  struct dht_bucket *m_cur;
};

struct db_chain *dbc_init (struct dht_bucket *);

struct dht_bucket *dbc_bucket (struct db_chain *);

struct dht_bucket *dbc_next (struct db_chain *);

#define dbc_cleanup(dc)     free (dc)

#endif
