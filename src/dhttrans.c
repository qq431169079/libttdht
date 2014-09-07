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
/* @CFILE dhttrans.c
*
*  Author: Alf <naihe2010@126.com>
*/
/* @date Created: 2009/08/19 11:16:40 Alf*/

#include "dhtlog.h"
#include "dhtbucket.h"
#include "dhtlib.h"
#include "dht.h"
#include "dhttrans.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

static struct dht_node_search_t *dsea_find_lower_bound (struct dht_search *,
							const char *);

struct dht_search *
dsea_init (const char *target, struct dht_bucket *contacts)
{
  struct dht_search *dsea;

  dsea = (struct dht_search *) calloc (1, sizeof (struct dht_search));
  assert (dsea);

  hashsg_cpy (dsea->m_target, target);

  dsea->m_next = NULL;
  dsea->m_pending = 0;
  dsea->m_contacted = 0;
  dsea->m_replied = 0;
  dsea->m_concurrency = 3;
  dsea->m_restart = 0;
  dsea->m_started = 0;

  dsea_add_contacts (dsea, contacts);

  return dsea;
}

void
dsea_claenup (struct dht_search *dsea)
{
  struct dht_node_search_t *dns;

  if (dsea->m_pending)
    {
      ttdht_err ("cleanup called with pending transactions.");
      return;
    }

  if (dsea->m_concurrency != 3)
    {
      ttdht_err ("with invalid concurrency limit.");
      return;
    }

  LIST_FOREACH (dns, &dsea->dht_node_search_list, entries)
  {
    LIST_REMOVE (dns, entries);
    dsea->dht_node_search_count--;
    free (dns);
  }
}

static struct dht_node_search_t *
dsea_find_lower_bound (struct dht_search *dsea, const char *id)
{
  struct dht_node_search_t *dnst, *smaller;

  smaller = NULL;
  dnst = LIST_FIRST (&dsea->dht_node_search_list);

  if (dnst == NULL)
    return NULL;

  while (dnst && hashsg_closer (dsea->m_target, dnst->node->hashsg, id))
    {
      smaller = dnst;
      dnst = LIST_NEXT (dnst, entries);
    }

  return smaller;
}

int
dsea_add_contact (struct dht_search *dsea, const char *id,
		  struct sockaddr *sa)
{
  struct dht_node_search_t *dns, *entry, *nentry;
  struct dht_node *n;

  entry = dsea_find_lower_bound (dsea, id);

  if (entry != NULL && (nentry = LIST_NEXT (entry, entries)) != NULL)
    {
      if (memcmp (&nentry->node->m_sockaddr, sa, sizeof (*sa)) == 0)
	{
	  return 0;
	}
    }

  n = dn_init (id, (struct sockaddr_in *) sa);
  assert (n);

  dns =
    (struct dht_node_search_t *) calloc (1,
					 sizeof (struct dht_node_search_t));
  assert (dns);

  dns->node = n;
  dns->search = dsea;

  if (entry == NULL)
    {
      LIST_INSERT_HEAD (&dsea->dht_node_search_list, dns, entries);
    }
  else
    {
      LIST_INSERT_AFTER (entry, dns, entries);
    }

  dsea->dht_node_search_count++;

  dsea->m_restart = 1;
  return 1;
}

void
dsea_add_contacts (struct dht_search *dsea, struct dht_bucket *contacts)
{
  struct dht_node *node;
  struct db_chain *chain;
  int needclosest, needgood;

  chain = dbc_init (contacts);

  needclosest = DSEARCH_MAX_CONTACTS - dsea->dht_node_search_count;
  needgood = DB_NUM_NODES;

  for (node = LIST_FIRST (chain->m_cur->m_nodes);
       needclosest > 0 || needgood > 0; node = LIST_NEXT (node, entries))
    {
      while (node == LIST_END (chain->m_cur->m_nodes))
	{
	  if (!dbc_next (chain))
	    {
	      dbc_cleanup (chain);
	      return;
	    }

	  node = LIST_FIRST (chain->m_cur->m_nodes);
	}

      if ((!DN_IS_BAD (node) || needclosest > 0)
	  && dsea_add_contact (dsea, node->hashsg,
			       (struct sockaddr *) &node->m_sockaddr))
	{
	  needgood -= !DN_IS_BAD (node);
	  needclosest--;
	}
    }

  dbc_cleanup (chain);
}

int
dsea_uncontacted (struct dht_search *dsea, struct dht_node *node)
{
  return (!DN_IS_ACTIVE (node)) && (!DN_IS_GOOD (node))
    && (!DN_IS_BAD (node));
}

struct dht_node_search_t *
dsea_get_contact (struct dht_search *dsea)
{
  struct dht_node_search_t *ret;

  if (dsea->m_pending >= dsea->m_concurrency)
    return NULL;

  if (dsea->m_restart)
    dsea_trim (dsea, 0);

  ret = dsea->m_next;
  if (ret == NULL)
    return ret;

  dsea_set_node_active (dsea, ret, 1);

  dsea->m_pending++;
  dsea->m_contacted++;

  while ((dsea->m_next = LIST_NEXT (dsea->m_next, entries)) != NULL)
    {
      if (dsea_uncontacted (dsea, dsea->m_next->node))
	break;
    }

  return ret;
}

void
dsea_node_status (struct dht_search *dsea, struct dht_node_search_t *node,
		  int suc)
{
  if (suc)
    {
      DN_SET_GOOD (node->node);
      dsea->m_replied++;
    }
  else
    {
      DN_SET_BAD (node->node);
    }

  dsea->m_pending--;
  dsea_set_node_active (dsea, node, 0);
}

void
dsea_trim (struct dht_search *dsea, int final)
{
  struct dht_node_search_t *dnst;

  int needclosest = final ? 0 : DSEARCH_MAX_CONTACTS;
  int needgood = dsea->is_anno ? DB_NUM_NODES : 0;

  dsea->m_next = NULL;

  for (dnst = LIST_FIRST (&dsea->dht_node_search_list); dnst != NULL;)
    {
      if ((!DN_IS_ACTIVE (dnst->node)) && needclosest <= 0
	  && ((!DN_IS_GOOD (dnst->node)) || needgood <= 0))
	{
	  struct dht_node_search_t *dnst2 = LIST_NEXT (dnst, entries);
	  dn_cleanup (dnst->node);
	  LIST_REMOVE (dnst, entries);
	  dsea->dht_node_search_count--;
	  free (dnst);
	  dnst = dnst2;
	  continue;
	}

      needclosest--;
      needgood -= DN_IS_GOOD (dnst->node);

      if (dsea->m_next == NULL && dsea_uncontacted (dsea, dnst->node))
	dsea->m_next = dnst;

      dnst = LIST_NEXT (dnst, entries);
    }

  dsea->m_restart = 0;
}

void
dsea_set_node_active (struct dht_search *dsea, struct dht_node_search_t *dnst,
		      int active)
{
  dnst->node->m_lastseen = active;
}

void
dsea_cleanup (struct dht_search *dsea)
{
  struct dht_node_search_t *n;

  while ((n = LIST_FIRST (&dsea->dht_node_search_list)) != NULL)
    {
      LIST_REMOVE (n, entries);
      dn_cleanup (n->node);
      free (n);
    }
}

struct dht_search *
dann_init (const char *key, const char *id,
	   struct dht_bucket *bucket, void (*cb) (const char *, const char *,
						  void *), void *arg)
{
  struct dht_search *ann;

  ann = dsea_init (id, bucket);
  assert (ann);

  ann->key = strdup (key);
  ann->is_anno = 1;
  ann->callback = cb;
  ann->arg = arg;

  return ann;
}

void
dann_cleanup (struct dht_search *dann)
{
  char *fail = NULL;

  if (dann->state != USR_ANNOUNCING)
    {
      if (!dann->m_contacted)
	fail = "No DHT nodes available for peer search.\n";
      else
	fail = "DHT search unsuccessful.\n";
    }
  else
    {
      if (!dann->m_contacted)
	fail = "DHT search unsucessful.\n";
      else
	fail = "Announce failed.\n";
    }

  if (fail)
    {
      ttdht_info ("%s\n", fail);
    }

  free (dann->key);
  free (dann);
}

struct dht_node_search_t *
dann_start_announce (struct dht_search *dann)
{
  struct dht_node_search_t *n;

  dsea_trim (dann, 1);

  if (LIST_EMPTY (&dann->dht_node_search_list))
    return NULL;

  if (!DSEA_COMPLETE (dann) || dann->m_next != NULL
      || dann->dht_node_search_count > DB_NUM_NODES)
    return NULL;

  dann->m_contacted = dann->m_pending = dann->dht_node_search_count;
  dann->m_replied = 0;
  dann->state = USR_ANNOUNCING;

  LIST_FOREACH (n, &dann->dht_node_search_list, entries)
  {
    dsea_set_node_active (dann, n, 1);
  }

  return LIST_FIRST (&dann->dht_node_search_list);
}

void
dann_receive_peers (struct dht_search *dann, struct dht_object *list)
{
  struct list_node *ln;
  struct string *str;

  LIST_FOREACH (ln, &list->m_list, entries)
  {
    if (dann->callback != NULL
	&& (str = OBJ_AS_STRING ((struct dht_object *) ln->item)) != NULL)
      {
	dann->callback (dann->key, str->data, dann->arg);
      }
  }
}

struct dht_trans *
dtr_init (int quicktimeout, int timeout, const char *id,
	  struct sockaddr_in *sa)
{
  struct dht_trans *dtr;
  time_t t;

  dtr = (struct dht_trans *) calloc (1, sizeof (struct dht_trans));
  assert (dtr);

  hashsg_cpy (dtr->m_id, id);
  dtr->m_has_quicktimeout = quicktimeout > 0;
  memcpy (&dtr->m_sa, sa, sizeof (struct sockaddr_in));

  time (&t);
  dtr->m_timeout = t + timeout;
  dtr->m_quicktimeout = t + quicktimeout;

  dtr->m_retry = 3;

  return dtr;
}

void
dtr_cleanup (struct dht_trans *dtr)
{
  string_clear (&dtr->m_token);
  free (dtr);
}

int
dtr_key (struct sockaddr_in *sa, int id)
{
  return (int) ((dht_trans_key_type) (sa->sin_addr.s_addr) << 32) + id;
}

int
dtr_key_match (dht_trans_key_type key, struct sockaddr_in *sa)
{
  return (key >> 32) == sa->sin_addr.s_addr;
}

struct dht_trans *
dts_init (int quicktimeout, int timeout, struct dht_node_search_t *node)
{
  struct dht_trans *dts;

  dts =
    dtr_init (quicktimeout, timeout, node->node->hashsg,
	      &node->node->m_sockaddr);
  assert (dts);

  dts->m_node = node;
  dts->m_search = node->search;

  if (!dts->m_has_quicktimeout)
    {
      dts->m_search->m_concurrency++;
    }

  return dts;
}

void
dts_set_stalled (struct dht_trans *dts)
{
  if (!dts->m_has_quicktimeout)
    {
      ttdht_err
	("DhtTransactionSearch::set_stalled called on already stalled transaction.");
      return;
    }

  dts->m_has_quicktimeout = 0;
  dts->m_search->m_concurrency++;
}

void
dts_complete (struct dht_trans *dts, int suc)
{
  if (dts->m_node == NULL)
    {
      ttdht_warn ("complete called multiple times.");
      return;
    }

  if (!dts->m_has_quicktimeout)
    dts->m_search->m_concurrency--;

  dsea_node_status (dts->m_search, dts->m_node, suc);

  dts->m_node = NULL;
}

void
dts_cleanup (struct dht_trans *dts)
{
  if (dts->m_node != NULL)
    dts_complete (dts, 0);

  if (dts->m_search)
    {
      if (DSEA_COMPLETE (dts->m_search))
	dsea_cleanup (dts->m_search);
    }

  dtr_cleanup (dts);
}

struct dht_trans *
dtan_init (const char *id, struct sockaddr_in *sa, char *target,
	   struct string *token)
{
  struct dht_trans *dtan;

  dtan = dtr_init (-1, 30, id, sa);
  assert (dtan);

  hashsg_cpy (dtan->m_info, target);
  string_cpy (&dtan->m_token, token);

  return dtan;
}
