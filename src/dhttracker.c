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
/* @CFILE dhttracker.c
*
*  Author: Alf <naihe2010@126.com>
*/
/* @date Created: 2009/08/19 11:16:40 Alf*/

#include "dhtlog.h"
#include "dhttracker.h"
#include "dhtlib.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>

struct dht_tracker *
dt_init (void)
{
  struct dht_tracker *dt;

  dt = (struct dht_tracker *) calloc (1, sizeof (struct dht_tracker));
  if (dt == NULL)
    {
      ttdht_err ("Memory error when create dht_tracker.\n");
      return NULL;
    }

  LIST_INIT (&dt->m_peers);

  return dt;
}

void
dt_add_peer (struct dht_tracker *dt, unsigned long ip, unsigned short port)
{
  struct dht_sockaddr *addr, *oldest = NULL;
  struct sockaddr_in laddr;
  time_t t, minseen = UINT_MAX;

  if (port == 0)
    return;

  laddr.sin_family = AF_INET;
  laddr.sin_addr.s_addr = ip;
  laddr.sin_port = port;
  memset (laddr.sin_zero, 0, 8);

  time (&t);
  LIST_FOREACH (addr, &dt->m_peers, entries)
  {
    if (addr->m_sa.sin_addr.s_addr == laddr.sin_addr.s_addr)
      {
	addr->m_sa.sin_port = laddr.sin_port;
	addr->m_lastseen = t;
	return;
      }
    else if (addr->m_lastseen < minseen)
      {
	minseen = addr->m_lastseen;
	oldest = addr;
      }
  }

  if (DTK_SIZE (dt) < DTK_MAX_PEERS)
    {
      addr = (struct dht_sockaddr *) calloc (1, sizeof (struct dht_sockaddr));
      if (addr == NULL)
	{
	  return;
	}

      memcpy (&addr->m_sa, &laddr, sizeof (struct sockaddr_in));
      addr->m_lastseen = t;
      LIST_INSERT_HEAD (&dt->m_peers, addr, entries);
      DTK_SIZE (dt)++;
    }
  else
    {
      oldest->m_sa.sin_addr.s_addr = laddr.sin_addr.s_addr;
      oldest->m_sa.sin_port = laddr.sin_port;
      oldest->m_lastseen = t;
    }
}

struct string *
dt_get_peers (struct dht_tracker *dt, unsigned int max)
{
  static struct string res;
  unsigned int i = 0, first = 0, last = 0, blocks;
  struct dht_sockaddr *dsa = LIST_FIRST (&dt->m_peers);

  if (DTK_SIZE (dt) > max)
    {
      blocks = (DTK_SIZE (dt) + max - 1) / max;
      first += (rand () % blocks) * (DTK_SIZE (dt) - max) / (blocks - 1);
      last = first + max;
    }

  while (i < first)
    dsa = LIST_NEXT (dsa, entries);

  memset (&res, 0, sizeof (res));
  string_ext (&res, max * 6 * sizeof (char));

  while (i < last)
    {
      memcpy (res.data + (i - first) * 6, &dsa->m_sa.sin_addr, 6);
      dsa = LIST_NEXT (dsa, entries);
    }

  return &res;
}

void
dt_prune (struct dht_tracker *dt, int maxage)
{
  struct dht_sockaddr *addr;
  time_t minseen = time (NULL) - maxage;

  LIST_FOREACH (addr, &dt->m_peers, entries)
  {
    if (addr->m_lastseen < minseen)
      {
	LIST_REMOVE (addr, entries);
	free (addr);
	DTK_SIZE (dt)--;
      }
  }
}

void
dt_cleanup (struct dht_tracker *tracker)
{
  free (tracker);
}
