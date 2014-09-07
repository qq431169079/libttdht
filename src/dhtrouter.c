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
/* @CFILE dhtrouter.c
*
*  Author: Alf <naihe2010@126.com>
*/
/* @date Created: 2009/08/19 11:16:40 Alf*/

#include "dhtlog.h"
#include "dhtbucket.h"
#include "dhttracker.h"
#include "dhttrans.h"
#include "dhtlib.h"
#include "dhtnode.h"
#include "dhtrouter.h"

#include <openssl/sha.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#ifdef WIN32
#include <time.h>
#include <winsock2.h>

#define timeradd(a, b, c) do {			\
    c->tv_sec = a->tv_sec + b->tv_sec;          \
    c->tv_usec = a->tv_usec + b->tv_usec;       \
    if (c->tv_usec > 1000000) {                 \
        c->tv_sec++;                            \
        c->tv_usec -= 1000000;                  \
    }                                           \
}                                               \
while (0)

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define EPOCHFILETIME  11644473600000000Ui64
#else /*  */
#define EPOCHFILETIME  11644473600000000ULL
#endif /*  */
struct timezone
{
  int tz_minuteswest;
  int tz_dsttime;
};
int
gettimeofday (struct timeval *tv, struct timezone *tz)
{
  FILETIME ft;
  LARGE_INTEGER li;
  __int64 t;
  static int tzflag;
  if (tv)

    {
      GetSystemTimeAsFileTime (&ft);
      li.LowPart = ft.dwLowDateTime;
      li.HighPart = ft.dwHighDateTime;
      t = li.QuadPart;		/* In 100-nanosecond intervals */
      t -= EPOCHFILETIME;	/* Offset to the Epoch time */
      t /= 10;			/* In microseconds */
      tv->tv_sec = (long) (t / 1000000);
      tv->tv_usec = (long) (t % 1000000);
    }
  if (tz)

    {
      if (!tzflag)

	{
	  _tzset ();
	  tzflag++;
	}
      tz->tz_minuteswest = _timezone / 60;
      tz->tz_dsttime = _daylight;
    }
  return 0;
}


#else /*  */
#include <sys/time.h>
#endif

static int dr_receive_timeout (struct dht_router *);
static int dr_receive_timeout_bootstrap (struct dht_router *);

char zero_id[HASH_STRING_LEN + 1] = { 0 };

struct dht_router *
dr_init (struct dht_object *cache, int port, dhtio_t *io)
{
  struct map *nodes;
  struct list *contacts;
  struct map_node *mn;
  struct list_node *ln;
  struct dht_router *dr;
  struct sockaddr_in addr;
  char ones_id[HASH_STRING_LEN + 1], buffer[HASH_STRING_LEN + 1];
  unsigned int i;
  struct string str, *temp;

  dr = (struct dht_router *) calloc (1, sizeof (struct dht_router));
  assert (dr);

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons (port);
  memset (&addr.sin_zero, 0, 8);

  dr->m_fdp = io->fdp;
  dr->read = io->read;
  dr->write = io->write;

  dr->node = dn_init (zero_id, &addr);
  dr->m_server = ds_init (dr);

  dr->m_numrefresh = 0;
  dr->m_curtoken = rand ();
  dr->m_prevtoken = rand ();

  string_set (&str, "self_id");
  if (cache && obj_has_key (cache, &str))
    {
      temp = obj_get_key_string (cache, &str);
      hashsg_cpy (dr->node->hashsg, temp->data);
    }
  else
    {
      for (i = 0; i < HASH_STRING_LEN; ++i)
	{
	  buffer[i] = rand ();
	}
      hashsg_init (buffer, HASH_STRING_LEN, dr->node->hashsg);
    }

  hashsg_clear (zero_id, 0);
  hashsg_clear (ones_id, 0xFF);
  dr->node->m_bucket = db_init (zero_id, ones_id);

  string_set2 (&str, ones_id, HASH_STRING_LEN);
  map_insert_head (&dr->m_buckets, &str, dr->node->m_bucket);

  string_set (&str, "nodes");
  if (cache && obj_has_key (cache, &str))
    {
      nodes = obj_get_key_map (cache, &str);

      LIST_FOREACH (mn, nodes, entries)
      {
	struct dht_node *node;
	node = dn_init_object (mn->key.data, (struct dht_object *) mn->value);
	map_insert_head (&dr->m_nodes, &mn->key, node);
	dr_add_node_to_bucket (dr, node);
      }
    }

  if (MAP_SIZE (&dr->m_nodes) < DR_NUM_BOOTSTRAP_COMPLETE)
    {
      string_set (&str, "contacts");
      if (cache && obj_has_key (cache, &str))
	{
	  struct string *addr = NULL;
	  int port = 0;
	  contacts = obj_get_key_list (cache, &str);

	  LIST_FOREACH (ln, contacts, entries)
	  {
	    struct list *contact =
	      OBJ_AS_LIST ((struct dht_object *) ln->item);
	    struct list_node *ln2;

	    LIST_FOREACH (ln2, contact, entries)
	    {
	      if (port == 0)
		port = (int) OBJ_AS_VALUE ((struct dht_object *) (ln2->item));
	      else
		addr = OBJ_AS_STRING ((struct dht_object *) ln2->item);

	      if (port && addr)
		{
		  map_insert_head (&dr->m_contacts, addr, (void *) port);
		}

	      port = 0;
	      addr = NULL;
	    }
	  }
	}
    }

  return dr;
}

void
dr_cleanup (struct dht_router *dr)
{
  struct map_node *mn;

  db_cleanup (dr->node->m_bucket);
  dn_cleanup (dr->node);
  ds_cleanup (dr->m_server);

  LIST_FOREACH (mn, &dr->m_nodes, entries)
  {
    dn_cleanup ((struct dht_node *) mn->value);
  }

  map_clear (&dr->m_nodes);

  map_clear (&dr->m_buckets);

  map_clear (&dr->m_trackers);

  map_clear (&dr->m_contacts);

  free (dr);
}

int
dr_start (struct dht_router *dr, int port)
{
  int ret;

  ret = ds_start (dr->m_server, port);
  if (ret < 0)
    {
      return -1;
    }

  dr->boot_timer =
    dr_timer_add (dr, 10, DHT_SOURCE (dr_receive_timeout_bootstrap), dr);
  return 0;
}

int
dr_run (struct dht_router *dr)
{
  struct timeval now[1];
  struct timer *tm1, *tm2;
  struct sockaddr_in sa[1];
  struct dht_action *act;
  char buf[1500];
  int ret;

  while (!dr->quit)
    {
      gettimeofday (now, NULL);

      /* 
       * check the timer list 
       * */
      for (tm1 = LIST_FIRST (dr->timer_list); tm1 != NULL; tm1 = tm2)
	{
	  tm2 = LIST_NEXT (tm1, entries);

	  if (timercmp (now, tm1->at, >))
	    {
//            ttdht_debug ("occured timer: %p\n", tm1);
	      if (tm1->cb (tm1->arg) == 0)
		{
		  LIST_REMOVE (tm1, entries);
		  free (tm1);
		}
	    }
	}

      /* 
       * wait some data 
       * */
      if (dr->m_server && dr->m_fdp)
	{
	  ret = dr->read (dr->m_fdp, buf, sizeof buf, sa, 10);
	  if (ret > 0)
	    {
	      ds_process (dr->m_server, sa, buf, ret);
	    }
	}

      /* 
       * check for action 
       * */
      while ((act = LIST_FIRST (dr->action_list)) != NULL)
	{
	  if (act->action == DHT_ACTION_PUB)
	    {
	      ds_announce (dr->m_server, act->actbuf, act->actport, 1, NULL,
			   NULL);
	    }
	  else if (act->action == DHT_ACTION_SEARCH)
	    {
	      ds_announce (dr->m_server, act->actbuf,
			   dr->m_server->port, 0, act->actcb,
			   act->actarg);
	    }
	  LIST_REMOVE (act, entries);
	  free (act);
	}
    }

  return 0;
}

void
dr_stop (struct dht_router *dr)
{
  if (dr->boot_timer != NULL)
    {
      dr_timer_remove (dr, dr->boot_timer);
      dr->boot_timer = NULL;
    }

  dr->quit = 1;
}

void
dr_announce (struct dht_router *dr, const char *key, void *util)
{
  ds_announce (dr->m_server, key, dr->m_server->port, 1,
	       NULL, util);
}

void
dr_pub (struct dht_router *dr, const char *key, unsigned short port)
{
  struct dht_action *act;

  act = calloc (1, sizeof (struct dht_action));
  if (act == NULL)
    {
      ttdht_err ("Memory error\n");
      return;
    }

  act->action = DHT_ACTION_PUB;
  sprintf (act->actbuf, "%s", key);
  act->actport = port;

  LIST_INSERT_HEAD (dr->action_list, act, entries);
}

void
dr_get (struct dht_router *dr, const char *key,
	void (*cb) (const char *, const char *, void *), void *arg)
{
  struct dht_action *act;

  act = calloc (1, sizeof (struct dht_action));
  if (act == NULL)
    {
      ttdht_err ("Memory error\n");
      return;
    }

  act->action = DHT_ACTION_SEARCH;
  sprintf (act->actbuf, "%s", key);
  act->actport = 0;
  act->actcb = cb;
  act->actarg = arg;

  LIST_INSERT_HEAD (dr->action_list, act, entries);
}

void
dr_cancel_announce (struct dht_router *dr, const char *id,
		    struct dht_tracker *tracker)
{
  ds_cancel_announce (dr->m_server, id, tracker);
}

struct dht_tracker *
dr_get_tracker (struct dht_router *dr, const char *id, int create)
{
  struct map_node *it;
  struct string str;

  LIST_FOREACH (it, &dr->m_trackers, entries)
  {
    if (strcmp (it->key.data, id) == 0)
      break;
  }

  if ((it != NULL))
    return (struct dht_tracker *) it->value;

  if (!create)
    return NULL;

  string_set2 (&str, id, HASH_STRING_LEN);
  it = map_node_init (&str, dt_init ());
  LIST_INSERT_HEAD (&dr->m_trackers, it, entries);
  dr->m_trackers.size++;

  return (struct dht_tracker *) it->value;
}

int
dr_want_node (struct dht_router *dr, const char *id)
{
  struct map_node *ib;
  if ((hashsg_cmp (id, dr->node->hashsg) == 0)
      || hashsg_cmp (id, zero_id) == 0)
    return 0;

  ib = dr_find_bucket (dr, id);
  return (struct dht_bucket *) ib->value == dr->node->m_bucket
    || DB_HAS_SPACE ((struct dht_bucket *) ib->value);
}

struct dht_node *
dr_get_node (struct dht_router *dr, const char *id)
{
  struct map_node *in;

  LIST_FOREACH (in, &dr->m_nodes, entries)
  {
    if (hashsg_cmp (in->key.data, id) == 0)
      break;
  }

  if (in == NULL)
    {
      if (hashsg_cmp (id, dr->node->hashsg) == 0)
	return dr->node;
      else
	return NULL;
    }

  return (struct dht_node *) in->value;
}

struct map_node *
dr_find_bucket (struct dht_router *dr, const char *id)
{
  struct map_node *ib, *bigger;

  bigger = LIST_FIRST (&dr->m_buckets);

  LIST_FOREACH (ib, &dr->m_buckets, entries)
  {
    if (hashsg_cmp (ib->key.data, id) == 0)
      return ib;

    if (hashsg_cmp (ib->key.data, id) > 0)
      {
	if (hashsg_cmp (ib->key.data, bigger->key.data) < 0)
	  {
	    bigger = ib;
	  }
      }
  }

  return bigger;
}

void
dr_add_contact (struct dht_router *dr, const char *host, int port)
{
  struct string str;

  if (dr->m_contacts_count >= DR_NUM_BOOTSTRAP_CONTACTS)
    {
      map_remove (&dr->m_contacts, map_begin (&dr->m_contacts));
    }

  string_set (&str, host);
  map_insert_head (&dr->m_contacts, &str, (void *) port);
}

void
dr_contact (struct dht_router *dr, struct sockaddr_in *sa, int port)
{
  if (DR_IS_ACTIVE (dr))
    {
      sa->sin_port = htons (port);
      ds_ping (dr->m_server, zero_id, sa);
    }
}

struct dht_node *
dr_node_queried (struct dht_router *dr, const char *id,
		 struct sockaddr_in *sa)
{
  struct dht_node *node;

  node = dr_get_node (dr, id);

  if (node == NULL)
    {
      if (dr_want_node (dr, id))
	{
	  ds_ping (dr->m_server, id, sa);
	}
      return NULL;
    }

  if (node->m_sockaddr.sin_addr.s_addr != sa->sin_addr.s_addr)
    return NULL;

  DN_QUERIED (node);
  if (DN_IS_GOOD (node))
    DB_TOUCH (node->m_bucket);

  return node;
}

struct dht_node *
dr_node_replied (struct dht_router *dr, const char *id,
		 const struct sockaddr_in *sa)
{
  struct dht_node *node;
  struct string str;

  node = dr_get_node (dr, id);

  if (node == NULL)
    {
      if (!dr_want_node (dr, id))
	return NULL;

      node = dn_init (id, sa);
      string_set2 (&str, id, HASH_STRING_LEN);
      map_insert_head (&dr->m_nodes, &str, node);

      if (!dr_add_node_to_bucket (dr, node))
	return NULL;
    }

  if (node->m_sockaddr.sin_addr.s_addr != sa->sin_addr.s_addr)
    return NULL;

  DN_REPLIED (node);
  DB_TOUCH (node->m_bucket);

  return node;
}

struct dht_node *
dr_node_inactive (struct dht_router *dr, const char *id,
		  const struct sockaddr_in *sa)
{
  struct dht_node *node;
  struct map_node *in;

  LIST_FOREACH (in, &dr->m_nodes, entries)
  {
    if (hashsg_cmp (in->key.data, id) == 0)
      break;
  }

  if (in == NULL)
    {
      return NULL;
    }

  node = (struct dht_node *) in->value;

  if (in == NULL || (node->m_sockaddr.sin_addr.s_addr != sa->sin_addr.s_addr))
    {
      return NULL;
    }

  DN_INACTIVE (node);

  if (DN_IS_BAD (node) && (DN_AGE (node) >= DR_TIMEOUT_REMOVE_NODE))
    {
      dr_delete_node (dr, in);
      return NULL;
    }

  return node;
}

void
dr_node_invalid (struct dht_router *dr, const char *id)
{
  struct map_node *in;
  struct dht_node *node;

  node = dr_get_node (dr, id);

  if (node == NULL || node == dr->node)
    return;

  LIST_FOREACH (in, &dr->m_nodes, entries)
  {
    if (hashsg_cmp (in->key.data, id) == 0)
      {
	dr_delete_node (dr, in);
	break;
      }
  }

}

char *
dr_store_closest_nodes (struct dht_router *dr, const char *id, char *buffer,
			char *bufferend)
{
  struct map_node *ib;
  struct db_chain *dc;
  struct dht_node *node;

  ib = dr_find_bucket (dr, id);
  dc = dbc_init ((struct dht_bucket *) ib->value);

  do
    {
      for (node = LIST_FIRST (dc->m_cur->m_nodes);
	   node != NULL && buffer != bufferend;
	   node = LIST_NEXT (node, entries))
	{
	  if (!DN_IS_BAD (node))
	    {
	      buffer = dn_store_compact (node, buffer);

	      if (buffer > bufferend)
		{
		  ttdht_err ("wrote past buffer end.");
		  dbc_cleanup (dc);
		  return NULL;
		}
	    }
	}
    }
  while (buffer < bufferend && dbc_next (dc) != NULL);

  dbc_cleanup (dc);
  return buffer;
}

static int
dr_receive_timeout_bootstrap (struct dht_router *dr)
{
  dr->boot_timer = NULL;

  if (MAP_SIZE (&dr->m_nodes) < DR_NUM_BOOTSTRAP_COMPLETE)
    {
      if (!MAP_EMPTY (&dr->m_nodes) || !MAP_EMPTY (&dr->m_contacts))
	dr_bootstrap (dr);

      dr->boot_timer = dr_timer_add (dr, DR_TIMEOUT_BOOTSTRAP_RETRY * 1000,
				     DHT_SOURCE
				     (dr_receive_timeout_bootstrap), dr);
      dr->m_numrefresh = 1;
    }
  else
    {
      map_clear (&dr->m_contacts);

      if (!dr->m_numrefresh)
	{
	  dr_receive_timeout (dr);
	}
      else
	{
	  dr->boot_timer = dr_timer_add (dr, DR_TIMEOUT_UPDATE * 1000,
					 DHT_SOURCE (dr_receive_timeout), dr);
	}

      dr->m_numrefresh = 2;
    }

  return 0;
}

static int
dr_receive_timeout (struct dht_router *dr)
{
  struct map_node *mn;

  dr->boot_timer = NULL;

  dr->m_prevtoken = dr->m_curtoken;
  dr->m_curtoken = rand ();

  LIST_FOREACH (mn, &dr->m_nodes, entries)
  {
    struct dht_node *node = (struct dht_node *) mn->value;
    DN_UPDATE (node);

    if (DN_IS_QUESTIONABLE (node)
	&& (DN_IS_BAD (node) || DN_AGE (node) >= DR_TIMEOUT_REMOVE_NODE))
      ds_ping (dr->m_server, node->hashsg, &node->m_sockaddr);
  }

  LIST_FOREACH (mn, &dr->m_buckets, entries)
  {
    struct dht_bucket *bucket = (struct dht_bucket *) mn->value;

    DB_UPDATE (bucket);

    if (!DB_IS_FULL (bucket) || DB_AGE (bucket) > DR_TIMEOUT_BUCKET_BOOTSTRAP)
      {
	dr_bootstrap_bucket (dr, bucket);
      }
  }

  LIST_FOREACH (mn, &dr->m_trackers, entries)
  {
    struct dht_tracker *tracker = (struct dht_tracker *) mn->value;

    dt_prune (tracker, DR_TIMEOUT_PEER_ANNOUNCE);

    if (DTK_EMPTY (tracker))
      {
	dt_cleanup (tracker);
	map_remove (&dr->m_trackers, mn);
      }
  }

  ds_update (dr->m_server);

  dr->m_numrefresh++;

  return 0;
}

char *
dr_generate_token (struct dht_router *dr, const struct sockaddr_in *sa,
		   int token, char *buffer)
{
  SHA_CTX ctx;
  unsigned long key = sa->sin_addr.s_addr;

  SHA1_Init (&ctx);
  SHA1_Update (&ctx, &token, sizeof (token));
  SHA1_Update (&ctx, &key, sizeof (key));
  SHA1_Final ((unsigned char *) buffer, &ctx);
  return buffer;
}

char *
dr_make_token (struct dht_router *dr, const struct sockaddr_in *sa,
	       char *buffer)
{
  return dr_generate_token (dr, sa, dr->m_curtoken, buffer);
}

int
dr_token_valid (struct dht_router *dr, const char *token,
		const struct sockaddr_in *sa)
{
  char reference[HASH_STRING_LEN + 1];

  if (memcmp
      (dr_generate_token (dr, sa, dr->m_curtoken, reference), token,
       DR_SIZE_TOKEN) == 0)
    return 1;

  return memcmp (dr_generate_token (dr, sa, dr->m_prevtoken, reference),
		 token, DR_SIZE_TOKEN) == 0;
}

struct dht_node *
dr_find_node (struct dht_router *dr, const struct sockaddr_in *sa)
{
  struct map_node *in;

  LIST_FOREACH (in, &dr->m_nodes, entries)
  {
    if (((struct dht_node *) in->value)->m_sockaddr.sin_addr.s_addr ==
	sa->sin_addr.s_addr)
      return (struct dht_node *) in->value;
  }

  return NULL;
}

struct map_node *
dr_split_bucket (struct dht_router *dr, struct map_node *ib,
		 struct dht_node *node)
{
  struct map_node *newib;
  struct dht_bucket *newbucket;
  struct string str;

  newbucket = db_split ((struct dht_bucket *) ib->value, dr->node->hashsg);

  if (dr->node->m_bucket->m_child != NULL)
    dr->node->m_bucket = dr->node->m_bucket->m_child;

  string_set2 (&str, newbucket->m_end, HASH_STRING_LEN);
  map_insert_before (&dr->m_buckets, ib, &str, newbucket);

  newib = NULL;

  if (DB_IS_INRANGE (newbucket, dr->node->hashsg))
    {
      if (DB_IS_EMPTY ((struct dht_bucket *) ib->value))
	{
	  dr_bootstrap_bucket (dr, (struct dht_bucket *) ib->value);
	}
    }
  else
    {
      if (DB_IS_EMPTY (newbucket))
	{
	  dr_bootstrap_bucket (dr, newbucket);
	}

      newib = ib;
    }

  return newib;
}

int
dr_add_node_to_bucket (struct dht_router *dr, struct dht_node *node)
{
  struct map_node *ib, *in;
  struct dht_node *bnode;

  ib = dr_find_bucket (dr, node->hashsg);

  while (DB_IS_FULL ((struct dht_bucket *) ib->value))
    {
      bnode = db_find_replacement ((struct dht_bucket *) ib->value, 0);

      if (DN_IS_BAD (bnode))
	{
	  LIST_FOREACH (in, &dr->m_nodes, entries)
	  {
	    if (hashsg_cmp (in->key.data, bnode->hashsg) == 0)
	      {
		dr_delete_node (dr, in);
		break;
	      }
	  }
	}
      else
	{
	  if ((struct dht_bucket *) ib->value != dr->node->m_bucket)
	    {
	      LIST_FOREACH (in, &dr->m_nodes, entries)
	      {
		if (hashsg_cmp (in->key.data, node->hashsg) == 0)
		  {
		    dr_delete_node (dr, in);
		    return 0;
		  }
	      }
	    }
	  ib = dr_split_bucket (dr, ib, node);
	}
    }

  db_add_node ((struct dht_bucket *) ib->value, node);
  node->m_bucket = (struct dht_bucket *) ib->value;

  return 0;
}

void
dr_delete_node (struct dht_router *dr, struct map_node *in)
{
  db_remove_node (((struct dht_node *) in->value)->m_bucket,
		  (struct dht_node *) in->value);
}

struct dht_object *
dr_store_cache (struct dht_router *dr, struct dht_object *container)
{
  struct dht_object *nodes, *contacts, *top;
  struct map_node *mn;
  struct string str, str2;

  string_set (&str, "self_id");
  string_set2 (&str2, dr->node->hashsg, HASH_STRING_LEN);
  obj_insert_key_string (container, &str, &str2);

  string_set (&str, "nodes");
  nodes = obj_init (OBJ_TYPE_MAP);
  obj_insert_key_object (container, &str, nodes);
  LIST_FOREACH (mn, &dr->m_nodes, entries)
  {
    if (!DN_IS_BAD ((struct dht_node *) mn->value))
      {
	top = obj_init (OBJ_TYPE_MAP);
	obj_insert_key_object (nodes, &mn->key, top);
	dn_store_cache ((struct dht_node *) mn->value, top);
      }
  }

  if (!MAP_EMPTY (&dr->m_contacts))
    {
      string_set (&str, "contacts");
      contacts = obj_init (OBJ_TYPE_LIST);
      obj_insert_key_object (container, &str, contacts);

      LIST_FOREACH (mn, &dr->m_contacts, entries)
      {
	top = obj_init (OBJ_TYPE_LIST);
	obj_insert_list_object (contacts, top);
	obj_insert_list_string (top, &mn->key);
	obj_insert_list_value (top, (signed long) mn->value);
      }
    }

  return container;
}

void
dr_bootstrap (struct dht_router *dr)
{
  struct dht_node *node;
  struct map_node *mn;
  int i, bu;

  i = 0;
  while (i++ < 8 && (mn = map_begin (&dr->m_contacts)))
    {
      struct sockaddr_in sa;
      sa.sin_family = AF_INET;
      sa.sin_addr.s_addr = inet_addr (mn->key.data);
      sa.sin_port = htons ((unsigned int) mn->value);
      memset (sa.sin_zero, 0, 8);
      dr_contact (dr, &sa, (unsigned int) mn->value);
      map_remove (&dr->m_contacts, mn);
    }

  if (LIST_EMPTY (&dr->m_nodes))
    return;

  dr_bootstrap_bucket (dr, dr->node->m_bucket);

  LIST_FOREACH (node, dr->node->m_bucket->m_nodes, entries)
  {
    if (DN_IS_GOOD (node))
      ds_ping (dr->m_server, node->hashsg, &node->m_sockaddr);
  }

  if (MAP_SIZE (&dr->m_buckets) < 2)
    return;

  mn = map_begin (&dr->m_buckets);
  bu = rand () % MAP_SIZE (&dr->m_buckets);
  while (mn != NULL && bu++ > 0)
    {
      mn = LIST_NEXT (mn, entries);
    }

  if (mn != NULL && mn->value != dr->node->m_bucket)
    {
      dr_bootstrap_bucket (dr, (struct dht_bucket *) mn->value);
    }
}

void
dr_bootstrap_bucket (struct dht_router *dr, struct dht_bucket *bucket)
{
  char contactid[HASH_STRING_LEN + 1];

  if (!DR_IS_ACTIVE (dr))
    return;

  if (bucket == dr->node->m_bucket)
    {
      hashsg_cpy (contactid, dr->node->hashsg);
      contactid[HASH_STRING_LEN - 1] ^= 1;
    }
  else
    {
      db_get_random_id (bucket, contactid);
    }

  ds_find_node (dr->m_server, bucket, contactid);
}

struct timer *
dr_timer_add (struct dht_router *dr, int msec, dht_source cb, void *arg)
{
  struct timer *timer;
  struct timeval now[1], add[1];

  timer = calloc (1, sizeof (struct timer));
  if (timer == NULL)
    {
      ttdht_err ("Memory error.\n");
      return NULL;
    }

  gettimeofday (now, NULL);

  add->tv_sec = msec / 1000;
  add->tv_usec = (msec % 1000) * 1000;
  timeradd (now, add, timer->at);

  timer->cb = cb;
  timer->arg = arg;

  LIST_INSERT_HEAD (dr->timer_list, timer, entries);

  return timer;
}

void
dr_timer_remove (struct dht_router *dr, struct timer *timer)
{
  LIST_REMOVE (timer, entries);
  free (timer);
}
