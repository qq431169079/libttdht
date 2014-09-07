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
/* @CFILE dhttrans.h
*
*  Author: Alf <naihe2010@126.com>
*/
/* @date Created: 2009/08/19 11:16:40 Alf*/

#ifndef _DHT_TRANS_H_
#define _DHT_TRANS_H_

#include "dhtlib.h"
#include "dhtnode.h"
#include "dhttracker.h"

#include <time.h>

#define DSEARCH_MAX_CONTACTS    18

struct dht_node_search_t
{
  struct dht_node *node;
  struct dht_search *search;
    LIST_ENTRY (dht_node_search_t) entries;
};

#define DSEA_NUM_CONTACTED(dsea)                   ((dsea)->m_contacted)
#define DSEA_NUM_REPLIED(dsea)                     ((dsea)->m_replied)

#define DSEA_START(dsea)                           ((dsea)->m_started = 1, (dsea)->m_pending)
#define DSEA_COMPLETE(dsea)                        ((dsea)->m_started && !(dsea)->m_pending)

#define DSEA_TARGET(dsea)                          ((dsea)->target)

enum
{ USR_IDLE, USR_SEARCHING, USR_ANNOUNCING } search_type;

struct dht_search
{
  LIST_HEAD (dht_node_search_list, dht_node_search_t) dht_node_search_list;
  unsigned int dht_node_search_count;
  struct dht_node_search_t *m_next;

  unsigned int m_pending;
  unsigned int m_contacted;
  unsigned int m_replied;
  unsigned int m_concurrency;

  int m_restart;
  int m_started;

  char m_target[HASH_STRING_LEN + 1];

  unsigned int state;
  int is_anno;

  int is_pub;
  char *key;
  unsigned short m_port;

  void (*callback) (const char *, const char *, void *);
  void *arg;
};

struct dht_search *dsea_init (const char *, struct dht_bucket *);
void dsea_cleanup (struct dht_search *);
void dsea_claenup (struct dht_search *);
int dsea_add_contact (struct dht_search *, const char *, struct sockaddr *);
void dsea_add_contacts (struct dht_search *, struct dht_bucket *);
int dsea_uncontacted (struct dht_search *, struct dht_node *);
struct dht_node_search_t *dsea_get_contact (struct dht_search *);
void dsea_node_status (struct dht_search *, struct dht_node_search_t *, int);
void dsea_trim (struct dht_search *, int);
void dsea_set_node_active (struct dht_search *, struct dht_node_search_t *,
			   int);

struct dht_search *dann_init (const char *, const char *, struct dht_bucket *,
			      void (*)(const char *, const char *, void *),
			      void *);
void dann_cleanup (struct dht_search *);
struct dht_node_search_t *dann_start_announce (struct dht_search *);
void dann_receive_peers (struct dht_search *, struct dht_object *);

#define DTP_HAS_TRANSACTION(dtp)                (dtp->m_id >= -1)
#define DTP_HAS_FAILED(dtp)                     (dtp->m_id == -1)
#define DTP_SET_FAILED(dtp)                     dtp->m_id = -1

#define DTP_ADDR(dtp)                           (&dtp->m_sa)
#define DTP_ID(dtp)                             (dtp->m_id)
#define DTP_AGE(dtp, t)                         (DTP_HAS_TRANSACTION(dtp)? 0: t + dtp->m_id)
#define DTP_TRAN(dtp)                           (dtp->m_trans)

typedef enum
{
  DHT_PING,
  DHT_FIND_NODE,
  DHT_GET_PEERS,
  DHT_ANNOUNCE_PEER,
} dht_trans_type;

#define DTRAN_ID(dt)                 ((dt)->m_id)
#define DTRAN_ADDR(dt)               ((dt)->&m_sa)
#define DTRAN_TIMEOUT(dt)            ((dt)->m_timeout)
#define DTRAN_QUICK_TIMEOUT(dt)      ((dt)->m_quicktimeout)
#define DTRAN_HAS_QUICK_TIMEOUT(dt)  ((dt)->m_has_quicktimeout)
#define DTRAN_DEC_RETRY(dt)          ((dt)->m_retry--)
#define DTRAN_RETRY(dt)              ((dt)->m_retry)
#define DTRAN_PACKET(dt)             ((dt)->m_packet)
#define DTRAN_SET_PACKET(dt, p)      do { (dt)->m_packet = p; } while (0)
#define DTRAN_KEY(dt, id)            dtran_key ((dt), &(dt)->m_sa, id)

typedef unsigned long long dht_trans_key_type;

struct dht_trans
{
  dht_trans_type type;

  dht_trans_key_type key;
  char m_id[HASH_STRING_LEN + 1];

  struct sockaddr_in m_sa;
  time_t m_timeout;
  time_t m_quicktimeout;
  int m_retry;

  int m_has_quicktimeout;

  struct dht_node_search_t *m_node;
  struct dht_search *m_search;

  char m_info[HASH_STRING_LEN + 1];
  struct string m_token;
};

struct dht_trans *dtr_init (int, int, const char *, struct sockaddr_in *);
int dtr_key (struct sockaddr_in *, int);
void dtr_cleanup (struct dht_trans *);
int dtr_key_match (dht_trans_key_type, struct sockaddr_in *);

struct dht_trans *dts_init (int, int, struct dht_node_search_t *);
void dts_set_stalled (struct dht_trans *);
void dts_complete (struct dht_trans *, int);
void dts_cleanup (struct dht_trans *);

struct dht_trans *dtan_init (const char *, struct sockaddr_in *, char *,
			     struct string *);

#endif
