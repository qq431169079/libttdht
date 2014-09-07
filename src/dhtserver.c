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
/* @CFILE dhtserver.c
*
*  Author: Alf <naihe2010@126.com>
*/
/* @date Created: 2009/08/19 11:16:40 Alf*/

#include "dhtlog.h"
#include "dhttrans.h"
#include "dhtbucket.h"
#include "dhtrouter.h"
#include "dhttrans.h"
#include "dht.h"
#include "dhtserver.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define PEER_VERSION "LT\x0C\x20"

#define HOST_MAX_IP          (ntohl (inet_addr ("239.255.255.255")))

static char *queries[] = {
  "ping",
  "find_node",
  "get_peers",
  "announce_peer",
};

extern char zero_id[];

static int ds_ontimer (struct dht_server *);

static void ds_write (struct dht_server *, struct sockaddr_in *,
		      struct dht_object *);

static int ds_encode_buf (struct dht_server *, struct dht_object *, char *,
			  int);

static struct dht_ttype_trans_t *ds_map_lower_bound (struct dht_server *,
						     dht_trans_key_type);
static struct dht_ttype_trans_t *ds_map_find (struct dht_server *,
					      dht_trans_key_type);

static void ds_reset_statistics (struct dht_server *);

static int ds_obj_valid (struct dht_server *, struct dht_object *);

static void ds_process_query (struct dht_server *, struct dht_object *,
			      const char *, struct sockaddr_in *sa,
			      struct dht_object *);
static void ds_process_response (struct dht_server *, int, const char *,
				 struct sockaddr_in *, struct dht_object *);
static void ds_process_error (struct dht_server *, int, struct sockaddr_in *,
			      struct dht_object *);

static void ds_parse_find_node_reply (struct dht_server *, struct dht_trans *,
				      struct string *);
static void ds_parse_find_node_reply2 (struct dht_server *,
				       struct dht_trans *, struct list *);
static void ds_parse_get_peers_reply (struct dht_server *, struct dht_trans *,
				      struct dht_object *);

static void ds_find_node_next (struct dht_server *, struct dht_trans *);

static void ds_create_query (struct dht_server *, struct dht_trans *, int,
			     struct sockaddr_in *, int);
static void ds_create_response (struct dht_server *, struct dht_object *,
				struct sockaddr_in *, struct dht_object *);

static void ds_create_find_node_response (struct dht_server *,
					  struct dht_object *,
					  struct dht_object *);
static void ds_create_get_peers_response (struct dht_server *,
					  struct dht_object *,
					  struct sockaddr_in *,
					  struct dht_object *);
static void ds_create_announce_peer_response (struct dht_server *,
					      struct dht_object *,
					      struct sockaddr_in *,
					      struct dht_object *);

static int ds_add_trans (struct dht_server *, struct dht_trans *, int);

static struct dht_ttype_trans_t *ds_failed_transaction (struct dht_server *,
							struct
							dht_ttype_trans_t *,
							int);

static void ds_clear_trans (struct dht_server *);

struct dht_server *
ds_init (struct dht_router *dr)
{
  struct dht_server *ds;

  ds = (struct dht_server *) calloc (1, sizeof (struct dht_server));
  assert (ds);

  ds->m_router = dr;

  ds->m_networkup = 0;

  ds_reset_statistics (ds);

  LIST_INIT (&ds->node_info_list);
  LIST_INIT (&ds->m_trans);

  return ds;
}

void
ds_cleanup (struct dht_server *ds)
{
  ds_stop (ds);
  free (ds);
}

void
ds_restart (struct dht_server *ds)
{
  ds_stop (ds);
  ds_start (ds, ds->port);
}

int
ds_start (struct dht_server *ds, unsigned short port)
{
  ttdht_info ("DHT Server started.\n");

  ds->port = port;

  ds->timer =
    dr_timer_add (ds->m_router, 3 * 1000, DHT_SOURCE (ds_ontimer), ds);

  return 0;
}

void
ds_stop (struct dht_server *ds)
{
  if (!DR_IS_ACTIVE (ds->m_router))
    return;

  if (ds->timer != NULL)
    {
      dr_timer_remove (ds->m_router, ds->timer);
      ds->timer = NULL;
    }

  ds->m_networkup = 0;

  ds_clear_trans (ds);
}

int
ds_process (struct dht_server *ds, struct sockaddr_in *rmt, char *buf,
	    int siz)
{
  struct dht_object *obj, *transid, *newtransid;
  int type = '?';
  char *nodeid = NULL, *cp;
  struct string str, *tpo;

  if (siz <= 0)
    {
      return 0;
    }

  string_set2 (&str, buf, siz);
  obj = buf_to_object (&str);
  if (obj == NULL)
    {
      ttdht_debug ("Create dht object error.\n");
      return 0;
    }

  if (!ds_obj_valid (ds, obj))
    {
      ttdht_debug ("Invalid dht object.\n");
      return 0;
    }

  string_set (&str, "t");
  if ((!OBJ_IS_MAP (obj)) || (!obj_has_key (obj, &str)))
    {
      ttdht_debug ("dht object is not valid.\n");
      obj_cleanup (obj);
      return 0;
    }

  string_set (&str, "t");
  transid = obj_get_key (obj, &str);

  string_set (&str, "y");
  if (!obj_has_key_string (obj, &str))
    {
      ttdht_debug ("No message type");
      obj_cleanup (obj);
      return 0;
    }

  string_set (&str, "y");
  tpo = obj_get_key_string (obj, &str);
  cp = tpo->data;
  type = cp[0];

  if (type == 'r' || type == 'q')
    {
      struct dht_object *temp;
      string_set (&str, type == 'q' ? "a" : "r");
      temp = obj_get_key (obj, &str);
      string_set (&str, "id");
      tpo = obj_get_key_string (temp, &str);

      if (tpo == NULL)
	{
	  ttdht_debug ("nodeid is NULL");
	}

      nodeid = tpo->data;
    }

  tpo = OBJ_AS_STRING (transid);

  switch (type)
    {
    case 'q':
      newtransid = obj_init (OBJ_TYPE_STRING);
      string_cpy (&newtransid->m_string, tpo);
      ds_process_query (ds, newtransid, nodeid, rmt, obj);
      break;

    case 'r':
      ds_process_response (ds, (unsigned char) tpo->data[0], nodeid, rmt,
			   obj);
      break;

    case 'e':
      ds_process_error (ds, (unsigned char) tpo->data[0], rmt, obj);
      break;

    default:
      ttdht_debug ("Unknown message type.");
    }

  obj_cleanup (obj);

  return 0;
}

static void
ds_reset_statistics (struct dht_server *ds)
{
  ds->m_queriesreceived = 0;
  ds->m_queriessent = 0;
  ds->m_repliesreceived = 0;
}

void
ds_ping (struct dht_server *ds, const char *id, struct sockaddr_in *sa)
{
  struct dht_ttype_trans_t *dtt;
  struct dht_trans *dtr;
  dht_trans_key_type rid;

  rid = dtr_key (sa, 0);
  dtt = ds_map_lower_bound (ds, rid);

  if (dtt == NULL || !dtr_key_match (dtt->key, sa))
    {
      dtr = dtr_init (-1, 30, id, sa);
      dtr->type = DHT_PING;
      ds_add_trans (ds, dtr, 0);
    }
}

void
ds_find_node (struct dht_server *ds, struct dht_bucket *contacts,
	      const char *target)
{
  struct dht_search *search;
  struct dht_node_search_t *ns;

  search = dsea_init (target, contacts);

  if (search == NULL)
    return;

  while ((ns = dsea_get_contact (search)) != NULL)
    {
      struct dht_trans *dts;
      dts = dts_init (4, 30, ns);
      dts->type = DHT_FIND_NODE;
      ds_add_trans (ds, dts, 0);
    }

  if (!DSEA_START (search))
    dsea_cleanup (search);
}

void
ds_announce (struct dht_server *ds, const char *key, unsigned short port,
	     int ispub, void (*cb) (const char *, const char *, void *),
	     void *arg)
{
  struct map_node *mn;
  struct dht_node_search_t *ns;
  struct dht_trans *dts;
  struct dht_search *announce;
  char info[HASH_STRING_LEN + 1];

  hashsg_init (key, (int) strlen (key), info);

  mn = dr_find_bucket (ds->m_router, info);
  if (mn == NULL)
    {
      ttdht_debug ("No buckets\n");
      return;
    }

  announce = dann_init (key, info, (struct dht_bucket *) mn->value, cb, arg);
  if (announce == NULL)
    {
      ttdht_debug ("create announce failed.\n");
      return;
    }

  announce->is_pub = ispub;
  announce->m_port = port;

  while ((ns = dsea_get_contact (announce)) != NULL)
    {
      dts = dts_init (4, 30, ns);
      dts->type = DHT_FIND_NODE;
      ds_add_trans (ds, dts, 0);
    }

  if (!DSEA_START (announce))
    {
      dann_cleanup (announce);
    }
}

void
ds_cancel_announce (struct dht_server *ds, const char *info, void *arg)
{
  struct dht_ttype_trans_t *dtt, *dtt2;

  while ((dtt = LIST_FIRST (&ds->m_trans)) != NULL)
    {
      struct dht_search *dsea;

      dsea = dtt->trans->m_search;
      if (dsea && dsea->is_anno == 1)
	{
	  if ((info == NULL || hashsg_cmp (dsea->m_target, info) == 0)
	      && (arg == NULL || dsea->arg == arg))
	    {
	      dts_cleanup (dtt->trans);
	      dtt2 = LIST_NEXT (dtt, entries);
	      LIST_REMOVE (dtt, entries);
	      free (dtt);
	      dtt = dtt2;
	      continue;
	    }
	}

      dtt = LIST_NEXT (dtt, entries);
    }
}

void
ds_update (struct dht_server *ds)
{
  ds->m_networkup = 0;
}

static void
ds_process_query (struct dht_server *ds, struct dht_object *transid,
		  const char *id, struct sockaddr_in *sa,
		  struct dht_object *msg)
{
  struct dht_object *arg, *reply;
  char *query;
  struct string str, *tpo;

  ds->m_queriesreceived++;
  ds->m_networkup = 1;

  string_set (&str, "q");
  tpo = obj_get_key_string (msg, &str);
  if (tpo == NULL)
    {
      ttdht_debug ("No query method.\n");
      return;
    }

  query = tpo->data;

  string_set (&str, "a");
  arg = obj_get_key (msg, &str);
  if (arg == NULL)
    {
      ttdht_debug ("No argument.\n");
      return;
    }

  reply = obj_init (OBJ_TYPE_MAP);

  if (!strcmp (query, "find_node"))
    ds_create_find_node_response (ds, arg, reply);
  else if (!strcmp (query, "get_peers"))
    ds_create_get_peers_response (ds, arg, sa, reply);
  else if (!strcmp (query, "announce_peer"))
    ds_create_announce_peer_response (ds, arg, sa, reply);
  else if (strcmp (query, "ping"))
    ttdht_debug ("Unknown query type.");

  dr_node_queried (ds->m_router, id, sa);

  ds_create_response (ds, transid, sa, reply);
}

static void
ds_create_find_node_response (struct dht_server *ds, struct dht_object *arg,
			      struct dht_object *reply)
{
  char compact[sizeof (struct compact_node_info) * DB_NUM_NODES];
  char *end;
  struct string str, str2, *tpo;

  string_set (&str, "target");
  tpo = obj_get_key_string (arg, &str);
  end =
    dr_store_closest_nodes (ds->m_router, tpo->data, compact,
			    compact + sizeof (compact));

  if (end == compact)
    {
      ttdht_debug ("No nodes.\n");
      return;
    }

  string_set (&str, "nodes");
  string_set2 (&str2, compact, end - compact);
  obj_insert_key_string (reply, &str, &str2);
}

static void
ds_create_get_peers_response (struct dht_server *ds, struct dht_object *arg,
			      struct sockaddr_in *sa,
			      struct dht_object *reply)
{
  char key[HASH_STRING_LEN * 2];
  struct dht_tracker *tracker;
  struct string str, str2, *info;

  dr_make_token (ds->m_router, sa, key);

  string_set (&str, "token");
  string_set2 (&str2, key, LOCAL_TOKEN_LEN);
  obj_insert_key_string (reply, &str, &str2);

  string_set (&str, "info_hash");
  info = obj_get_key_string (arg, &str);

  tracker = dr_get_tracker (ds->m_router, info->data, 0);

  if (!tracker || DTK_EMPTY (tracker))
    {
      char compact[sizeof (struct compact_node_info) * DB_NUM_NODES];
      char *end = dr_store_closest_nodes (ds->m_router, info->data, compact,
					  compact + sizeof (compact));

      if (end == compact)
	{
	  ttdht_debug ("No peers nor nodes.\n");
	  return;
	}

      string_set (&str, "nodes");
      string_set2 (&str2, compact, end - compact);
      obj_insert_key_string (reply, &str, &str2);
    }
  else
    {
      struct dht_object *values;
      struct string *peers;
      values = obj_init (OBJ_TYPE_LIST);
      peers = dt_get_peers (tracker, DTK_MAX_PEERS);
      obj_insert_list_string (values, peers);
      string_clear (peers);
      string_set (&str, "values");
      obj_insert_key_object (reply, &str, values);
    }
}

static void
ds_create_announce_peer_response (struct dht_server *ds,
				  struct dht_object *arg,
				  struct sockaddr_in *sa,
				  struct dht_object *reply)
{
  struct dht_tracker *tracker;
  struct string str, *info, *token;

  string_set (&str, "info_hash");
  info = obj_get_key_string (arg, &str);

  string_set (&str, "token");
  token = obj_get_key_string (arg, &str);
  if (!dr_token_valid (ds->m_router, token->data, sa))
    {
      ttdht_debug ("Token invalid.\n");
      return;
    }

  tracker = dr_get_tracker (ds->m_router, info->data, 1);

  string_set (&str, "port");
  dt_add_peer (tracker, sa->sin_addr.s_addr,
	       htons (obj_get_key_value (arg, &str)));
}

static void
ds_process_response (struct dht_server *ds, int transid, const char *id,
		     struct sockaddr_in *sa, struct dht_object *req)
{
  struct dht_trans *dtr;
  struct dht_ttype_trans_t *dtt;
  struct dht_object *res;
  dht_trans_key_type key;
  struct string str, *snodes;
  struct list *snodes2;

  key = dtr_key (sa, transid);
  dtt = ds_map_find (ds, key);
  if (dtt == NULL)
    {
      return;
    }

  ds->m_repliesreceived++;
  ds->m_networkup = 1;

  dtr = dtt->trans;

  if (hashsg_cmp (id, dtr->m_id) != 0 && hashsg_cmp (dtr->m_id, zero_id) != 0)
    return;

  string_set (&str, "r");
  res = obj_get_key (req, &str);
  if (res == NULL)
    {
      ttdht_debug ("No reponse found.\n");
      return;
    }

  switch (dtr->type)
    {
    case DHT_FIND_NODE:
      string_set (&str, "nodes");
      snodes = obj_get_key_string (res, &str);
      if (snodes != NULL)
	{
	  ds_parse_find_node_reply (ds, dtr, snodes);
	}
      string_set (&str, "nodes2");
      snodes2 = obj_get_key_list (res, &str);
      if (snodes2 != NULL)
	{
	  ds_parse_find_node_reply2 (ds, dtr, snodes2);
	}
      break;

    case DHT_GET_PEERS:
      ds_parse_get_peers_reply (ds, dtr, res);
      break;

    default:
      break;
    }

  dr_node_replied (ds->m_router, id, sa);

  dts_cleanup (dtr);
  LIST_REMOVE (dtt, entries);
  free (dtt);
}

static void
ds_process_error (struct dht_server *ds, int transid, struct sockaddr_in *sa,
		  struct dht_object *req)
{
  dht_trans_key_type key;
  struct dht_ttype_trans_t *dtt;

  key = dtr_key (sa, transid);
  dtt = ds_map_find (ds, key);

  if (dtt == NULL)
    return;

  ds->m_repliesreceived++;
  ds->m_networkup = 1;

  LIST_REMOVE (dtt, entries);
  dtr_cleanup (dtt->trans);
  free (dtt);
}

static void
ds_parse_find_node_reply (struct dht_server *ds, struct dht_trans *dts,
			  struct string *nodes)
{
  int size, i;
  char buf[32];

  dts_complete (dts, 1);

  size = nodes->len / 26;
  for (i = 0; i < size; ++i)
    {
      char *p = (char *) nodes->data + i * 26;
      if (hashsg_cmp (p, ds->m_router->node->hashsg) != 0)
	{
	  struct sockaddr_in sa[1];
	  sa->sin_family = AF_INET;
	  memcpy (&sa->sin_addr, p + HASH_STRING_LEN, 4);
	  memcpy (&sa->sin_port, p + HASH_STRING_LEN + 4, 2);
	  memset (sa->sin_zero, 0, 8);
	  if (ntohl (sa->sin_addr.s_addr) >= HOST_MAX_IP)
	    {
	      ttdht_debug ("Invalid IP or Port [%s:%d].\n",
			   inet_ntoa (sa->sin_addr), ntohs (sa->sin_port));
	      continue;
	    }
//        inet_ntop (AF_INET, &sa->sin_addr, buf, sizeof buf);
	  strcpy (buf, inet_ntoa (sa->sin_addr));
	  ttdht_debug ("Add contact [%s:%d].\n", buf, ntohs (sa->sin_port));
	  dsea_add_contact (dts->m_search, p, (struct sockaddr *) sa);
	}
    }

  ds_find_node_next (ds, dts);
}

static void
ds_parse_find_node_reply2 (struct dht_server *ds, struct dht_trans *dts,
			   struct list *list)
{
  struct string *str;
  struct dht_object *obj;
  struct list_node *ln;
  char buf[32];

  dts_complete (dts, 1);

  LIST_FOREACH (ln, list, entries)
  {
    struct sockaddr_in sa[1];
    obj = ln->item;

    if (obj == NULL || (str = OBJ_AS_STRING (obj)) == NULL)
      {
	continue;
      }

    sa->sin_family = AF_INET;
    memcpy (&sa->sin_addr, str->data + HASH_STRING_LEN + 12, 4);
    memcpy (&sa->sin_port, str->data + HASH_STRING_LEN + 16, 2);
    memset (sa->sin_zero, 0, 8);
    //inet_ntop (AF_INET, &sa->sin_addr, buf, sizeof buf);
    strcpy (buf, inet_ntoa (sa->sin_addr));
    ttdht_debug ("Add contact [%s:%d].\n", buf, ntohs (sa->sin_port));
    dsea_add_contact (dts->m_search, str->data, (struct sockaddr *) sa);
  }

  ds_find_node_next (ds, dts);
}

static void
ds_parse_get_peers_reply (struct dht_server *ds, struct dht_trans *dtr,
			  struct dht_object *res)
{
  struct dht_object *list;
  struct dht_search *dann;
  struct string str;

  dann = (struct dht_search *) dtr->m_search;

  dts_complete (dtr, 1);

  string_set (&str, "values");
  if (obj_has_key_list (res, &str))
    {
      list = obj_get_key (res, &str);
      dann_receive_peers (dann, list);
    }

  if (dann->is_pub)
    {
      string_set (&str, "token");
      if (obj_has_key_string (res, &str))
	{
	  struct dht_trans *dtan;
	  struct string *tpo;

	  tpo = obj_get_key_string (res, &str);
	  dtan = dtan_init (dtr->m_id, &dtr->m_sa, dann->m_target, tpo);
	  dtan->type = DHT_ANNOUNCE_PEER;
	  dtan->m_search = dann;

	  ds_add_trans (ds, dtan, 0);
	}
    }
}

static void
ds_find_node_next (struct dht_server *ds, struct dht_trans *dts)
{
  struct dht_node_search_t *dns;
  struct dht_search *dann;

  while ((dns = dsea_get_contact (dts->m_search)) != NULL)
    {
      struct dht_trans *dtr;
      dtr = dts_init (4, 30, dns);
      dtr->type = DHT_FIND_NODE;
      ds_add_trans (ds, dtr, 0);
    }

  if (!dts->m_search->is_anno)
    return;

  dann = (struct dht_search *) dts->m_search;

  ttdht_debug ("restart: %d, pendding: %d\n", dann->m_started,
	       dann->m_pending);
  if (DSEA_COMPLETE (dann))
    {
      for (dns = dann_start_announce (dann); dns != NULL;
	   dns = LIST_NEXT (dns, entries))
	{
	  struct dht_trans *dtr;
	  dtr = dts_init (-1, 30, dns);
	  dtr->type = DHT_GET_PEERS;
	  ds_add_trans (ds, dtr, 0);
	}
    }
}

static void
ds_create_query (struct dht_server *ds, struct dht_trans *dtr, int transid,
		 struct sockaddr_in *sa, int pri)
{
  char trans_id[3];
  struct dht_object *query, *q;
  struct string str, str2;

  if (hashsg_cmp (dtr->m_id, ds->m_router->node->hashsg) == 0)
    return;

  query = obj_init (OBJ_TYPE_MAP);
  sprintf (trans_id, "%c", transid);

  string_set (&str, "t");
  string_set2 (&str2, trans_id, 1);
  obj_insert_key_string (query, &str, &str2);

  string_set (&str, "y");
  string_set (&str2, "q");
  obj_insert_key_string (query, &str, &str2);

  string_set (&str, "q");
  string_set (&str2, queries[dtr->type]);
  obj_insert_key_string (query, &str, &str2);

  string_set (&str, "v");
  string_set (&str2, PEER_VERSION);
  obj_insert_key_string (query, &str, &str2);

  q = obj_init (OBJ_TYPE_MAP);
  string_set (&str, "id");
  string_set2 (&str2, ds->m_router->node->hashsg, HASH_STRING_LEN);
  obj_insert_key_string (q, &str, &str2);

  string_set (&str, "a");
  obj_insert_key_object (query, &str, q);

  switch (dtr->type)
    {
    case DHT_PING:
      break;

    case DHT_FIND_NODE:
      string_set (&str, "target");
      string_set2 (&str2, dtr->m_search->m_target, HASH_STRING_LEN);
      obj_insert_key_string (q, &str, &str2);
      break;

    case DHT_GET_PEERS:
      string_set (&str, "info_hash");
      string_set2 (&str2, dtr->m_search->m_target, HASH_STRING_LEN);
      obj_insert_key_string (q, &str, &str2);
      ttdht_debug ("get key: %s from %s:%d\n", dtr->m_search->key, inet_ntoa (dtr->m_sa.sin_addr), ntohs (dtr->m_sa.sin_port));	// for debug
      break;

    case DHT_ANNOUNCE_PEER:
      string_set (&str, "info_hash");
      string_set2 (&str2, dtr->m_info, HASH_STRING_LEN);
      obj_insert_key_string (q, &str, &str2);

      string_set (&str, "token");
      obj_insert_key_string (q, &str, &dtr->m_token);

      string_set (&str, "port");
      obj_insert_key_value (q, &str, dtr->m_search->m_port);

      ttdht_debug ("pub key: %s with port: %d on %s:%d\n", dtr->m_search->key, dtr->m_search->m_port, inet_ntoa (dtr->m_sa.sin_addr), ntohs (dtr->m_sa.sin_port));	// for debug
      break;
    }

  ds->m_queriessent++;

  ttdht_debug ("dht server send query: %d to %s:%d\n", dtr->type,
	       inet_ntoa (dtr->m_sa.sin_addr), ntohs (dtr->m_sa.sin_port));
  ds_write (ds, &dtr->m_sa, query);

  obj_cleanup (query);
}

static void
ds_create_response (struct dht_server *ds, struct dht_object *transid,
		    struct sockaddr_in *sa, struct dht_object *res)
{
  struct dht_object *reply;
  struct string str, str2;

  reply = obj_init (OBJ_TYPE_MAP);

  string_set (&str, "id");
  string_set2 (&str2, ds->m_router->node->hashsg, HASH_STRING_LEN);
  obj_insert_key_string (res, &str, &str2);

  string_set (&str, "t");
  obj_insert_key_object (reply, &str, transid);

  string_set (&str, "y");
  string_set (&str2, "r");
  obj_insert_key_string (reply, &str, &str2);

  string_set (&str, "r");
  obj_insert_key_object (reply, &str, res);

  string_set (&str, "v");
  string_set (&str2, PEER_VERSION);
  obj_insert_key_string (reply, &str, &str2);

  ds_write (ds, sa, reply);

  obj_cleanup (reply);
}

static struct dht_ttype_trans_t *
ds_map_lower_bound (struct dht_server *ds, dht_trans_key_type key)
{
  struct dht_ttype_trans_t *dtt, *bigger;

  bigger = LIST_FIRST (&ds->m_trans);

  LIST_FOREACH (dtt, &ds->m_trans, entries)
  {
    if (dtt->key == key)
      return dtt;

    if (dtt->key > key)
      {
	if (dtt->key < bigger->key)
	  {
	    bigger = dtt;
	  }
      }
  }

  return bigger;
}

static struct dht_ttype_trans_t *
ds_map_find (struct dht_server *ds, dht_trans_key_type key)
{
  struct dht_ttype_trans_t *dtt;

  LIST_FOREACH (dtt, &ds->m_trans, entries)
  {
    if (dtt->key == key)
      return dtt;
  }

  return NULL;
}

static int
ds_add_trans (struct dht_server *ds, struct dht_trans *dtr, int pri)
{
  struct dht_ttype_trans_t *dtt;
  unsigned int rnd, id;
  dht_trans_key_type key;

  rnd = (unsigned char) rand ();
  id = rnd;

  key = dtr_key (&dtr->m_sa, id);
  dtt = ds_map_lower_bound (ds, key);

  while (dtt != NULL && dtt->key == key)
    {
      dtt = LIST_NEXT (dtt, entries);
      id = (unsigned char) (id + 1);

      if (id == rnd)
	{
	  dtr_cleanup (dtr);
	  return -1;
	}

      if (id == 0)
	{
	  key = dtr_key (&dtr->m_sa, id);
	  dtt = ds_map_lower_bound (ds, key);
	}
    }

  dtt =
    (struct dht_ttype_trans_t *) calloc (1,
					 sizeof (struct dht_ttype_trans_t));
  assert (dtt);
  dtt->key = key;
  dtt->trans = dtr;
  LIST_INSERT_HEAD (&ds->m_trans, dtt, entries);

  ds_create_query (ds, dtr, id, &dtr->m_sa, pri);

  return 0;
}

static int
ds_ontimer (struct dht_server *ds)
{
  struct dht_ttype_trans_t *dtt;
  struct dht_trans *dtr;
  time_t t;

  time (&t);
  for (dtt = LIST_FIRST (&ds->m_trans); dtt != NULL;)
    {
      dtr = dtt->trans;
      if (dtr->m_has_quicktimeout && dtr->m_quicktimeout < t)
	{
	  dtt = ds_failed_transaction (ds, dtt, 1);
	  continue;
	}
      else if (dtr->m_timeout < t)
	{
	  dtt = ds_failed_transaction (ds, dtt, 0);
	  continue;
	}
      else
	{
	  dtt = LIST_NEXT (dtt, entries);
	}
    }

  return 1;
}

static struct dht_ttype_trans_t *
ds_failed_transaction (struct dht_server *ds, struct dht_ttype_trans_t *dtt,
		       int quick)
{
  struct dht_ttype_trans_t *dtt2;
  struct dht_trans *dtr;

  dtr = dtt->trans;

  if (!quick && ds->m_networkup && (hashsg_cmp (dtr->m_id, zero_id) != 0))
    dr_node_inactive (ds->m_router, dtr->m_id, &dtr->m_sa);

  if (dtr->type == DHT_FIND_NODE)
    {
      if (quick)
	dts_set_stalled (dtr);
      else
	{
	  dts_complete (dtr, 0);
	}

      ds_find_node_next (ds, dtr);
    }

  dtt2 = LIST_NEXT (dtt, entries);
  if (!quick)
    {
      dts_cleanup (dtt->trans);
      LIST_REMOVE (dtt, entries);
      free (dtt);
    }

  return dtt2;
}

static void
ds_clear_trans (struct dht_server *ds)
{
  struct dht_ttype_trans_t *dtt;

  while ((dtt = LIST_FIRST (&ds->m_trans)) != NULL)
    {
      dtr_cleanup (dtt->trans);
      LIST_REMOVE (dtt, entries);
      free (dtt);
    }
}

static int
ds_encode_buf (struct dht_server *ds, struct dht_object *obj, char *buf,
	       int siz)
{
  struct string str;
  int ret;

  string_set2 (&str, buf, siz);
  ret = object_to_buf (obj, &str);
  if (ret < 0)
    {
      ttdht_debug ("decode dht object to buffer error.\n");
      return -1;
    }

  ret = str.data - buf;

  return ret;
}

static void
ds_write (struct dht_server *ds, struct sockaddr_in *sa,
	  struct dht_object *obj)
{
  char buf[1500];
  int ret;

  if (ntohl (sa->sin_addr.s_addr) >= HOST_MAX_IP)
    {
      ttdht_debug ("Invalid IP or Port [%s:%d], can't send message.\n",
		   inet_ntoa (sa->sin_addr), ntohs (sa->sin_port));
      return;
    }

  ret = ds_encode_buf (ds, obj, buf, sizeof buf);

  if (ret < 0)
    {
      ttdht_debug ("encode error.\n");
      return;
    }

  ds->m_router->write (ds->m_router->m_fdp, buf, ret, sa);
}

static int
ds_obj_valid (struct dht_server *ds, struct dht_object *obj)
{
  struct string str, *hash;
  int i;
  static const char *hashname[] = { "id", "target", "info_hash" };

  for (i = 0; i < sizeof (hashname) / sizeof (hashname[0]); ++i)
    {
      string_set (&str, hashname[i]);
      if (obj_has_key (obj, &str))
	{
	  hash = obj_get_key_string (obj, &str);
	  if (hash == NULL || hash->len != HASH_STRING_LEN)
	    return 0;
	}
    }

  return 1;
}
