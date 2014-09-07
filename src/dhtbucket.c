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
/* @CFILE dhtbucket.c
*
*  Author: Alf <naihe2010@126.com>
*/
/* @date Created: 2009/08/19 11:16:40 Alf*/

#include "dhtbucket.h"
#include "dhtnode.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <time.h>

struct dht_bucket *
db_init (const char *begin, const char *end)
{
  struct dht_bucket *db;

  db = (struct dht_bucket *) calloc (1, sizeof (struct dht_bucket));
  assert (db);

  db->m_parent = NULL;
  db->m_child = NULL;

  LIST_INIT (db->m_nodes);

  db->m_lastchanged = time (NULL);

  db->m_good = 0;
  db->m_bad = 0;

  hashsg_cpy (db->m_begin, begin);
  hashsg_cpy (db->m_end, end);

  return db;
}

void
db_cleanup (struct dht_bucket *db)
{
  struct dht_node *dn;

  while ((dn = LIST_FIRST (db->m_nodes)) != NULL)
    {
      LIST_REMOVE (dn, entries);
      /* dn_cleanup (dn); */
    }

  free (db);
}

void
db_add_node (struct dht_bucket *db, struct dht_node *n)
{
  LIST_INSERT_HEAD (db->m_nodes, n, entries);

  DB_TOUCH (db);

  if (DN_IS_GOOD (n))
    {
      db->m_good++;
    }
  else if (DN_IS_BAD (n))
    {
      db->m_bad++;
    }
}

void
db_remove_node (struct dht_bucket *db, struct dht_node *n)
{
  LIST_REMOVE (n, entries);

  if (DN_IS_GOOD (n))
    {
      db->m_good--;
    }
  else if (DN_IS_BAD (n))
    {
      db->m_bad--;
    }
}

void
db_count (struct dht_bucket *db)
{
  struct dht_node *n;

  db->m_good = 0;
  db->m_bad = 0;

  LIST_FOREACH (n, db->m_nodes, entries)
  {
    if (DN_IS_GOOD (n))
      db->m_good++;
    if (DN_IS_BAD (n))
      db->m_bad++;
  }
}

struct dht_node *
db_find_replacement (struct dht_bucket *db, int onlyoldest)
{
  struct dht_node *n, *oldest;
  time_t oldesttime;

  oldest = NULL;
  oldesttime = UINT_MAX;
  LIST_FOREACH (n, db->m_nodes, entries)
  {
    if (DN_IS_BAD (n) && !onlyoldest)
      return n;

    if (n->m_lastseen < oldesttime)
      {
	oldesttime = n->m_lastseen;
	oldest = n;
      }
  }

  return oldest;
}

void
db_get_mid_point (struct dht_bucket *db, char *middle_id)
{
  unsigned int i;

  hashsg_cpy (middle_id, db->m_end);

  for (i = 0; i < HASH_STRING_LEN; i++)
    {
      if (db->m_begin[i] != db->m_end[i])
	{
	  middle_id[i] =
	    ((unsigned char) db->m_begin[i] +
	     (unsigned char) db->m_end[i]) / 2;
	  break;
	}
    }
}

void
db_get_random_id (struct dht_bucket *db, char *rand_id)
{
  unsigned int i;

  hashsg_cpy (rand_id, db->m_end);

  for (i = 0; i < HASH_STRING_LEN; i++)
    rand_id[i] =
      db->m_begin[i] + (rand () & (db->m_end[i] - db->m_begin[i]));
}

struct dht_bucket *
db_split (struct dht_bucket *db, const char *hashsg)
{
  struct dht_node *n;
  struct dht_bucket *new;
  char mid_range[HASH_STRING_LEN + 1];
  unsigned int i, sum;
  int carry;

  db_get_mid_point (db, mid_range);

  new = db_init (db->m_begin, mid_range);

  carry = 1;
  for (i = HASH_STRING_LEN; i > 0; i--)
    {
      sum = (unsigned char) mid_range[i - 1] + carry;
      db->m_begin[i - 1] = (unsigned char) sum;
      carry = sum >> 8;
    }

  LIST_FOREACH (n, db->m_nodes, entries)
  {
    if (DB_IS_INRANGE (new, n->hashsg))
      {
	LIST_REMOVE (n, entries);

	LIST_INSERT_HEAD (new->m_nodes, n, entries);
	n->m_bucket = new;
      }
  }

  new->m_lastchanged = db->m_lastchanged;

  db_count (new);
  db_count (db);

  if (DB_IS_INRANGE (new, hashsg))
    {
      db->m_child = new;
      new->m_parent = db;
    }
  else
    {
      if (db->m_parent != NULL)
	{
	  db->m_parent->m_child = new;
	  new->m_parent = db->m_parent;
	}

      db->m_parent = new;
      new->m_child = db;
    }

  return new;
}

struct db_chain *
dbc_init (struct dht_bucket *db)
{
  struct db_chain *dc;

  dc = (struct db_chain *) calloc (1, sizeof (struct db_chain));
  assert (dc);

  dc->m_restart = db;
  dc->m_cur = db;

  return dc;
}

struct dht_bucket *
dbc_bucket (struct db_chain *dc)
{
  return dc->m_cur;
}

struct dht_bucket *
dbc_next (struct db_chain *dc)
{
  if (dc->m_restart == NULL)
    {
      dc->m_cur = dc->m_cur->m_parent;
    }
  else
    {
      dc->m_cur = dc->m_cur->m_child;

      if (dc->m_cur == NULL)
	{
	  dc->m_cur = dc->m_restart->m_parent;
	  dc->m_restart = NULL;
	}
    }

  return dc->m_cur;
}
