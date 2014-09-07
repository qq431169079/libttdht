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
/* @CFILE dhtnode.c
*
*  Author: Alf <naihe2010@126.com>
*/
/* @date Created: 2009/08/19 11:16:40 Alf*/

#include "dhtnode.h"
#include "dhtlib.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct dht_node *
dn_init (const char *id, const struct sockaddr_in *sa)
{
  struct dht_node *dn;

  dn = (struct dht_node *) calloc (1, sizeof (struct dht_node));
  assert (dn);

  hashsg_cpy (dn->hashsg, id);

  memcpy (&dn->m_sockaddr, sa, sizeof (struct sockaddr_in));
  dn->m_lastseen = 0;
  dn->m_active = 0;
  dn->m_inactive = 0;
  dn->m_bucket = NULL;

  return dn;
}

struct dht_node *
dn_init_object (const char *id, struct dht_object *obj)
{
  struct dht_node *dn;
  struct string str;

  dn = (struct dht_node *) calloc (1, sizeof (struct dht_node));
  assert (dn);

  hashsg_cpy (dn->hashsg, id);

  dn->m_lastseen = 0;
  dn->m_active = 0;
  dn->m_inactive = 0;
  dn->m_bucket = NULL;

  dn->m_sockaddr.sin_family = AF_INET;

  string_set (&str, "i");
  dn->m_sockaddr.sin_addr.s_addr = obj_get_key_value (obj, &str);

  string_set (&str, "p");
  dn->m_sockaddr.sin_port = (unsigned short) obj_get_key_value (obj, &str);

  string_set (&str, "t");
  dn->m_lastseen = obj_get_key_value (obj, &str);

  DN_UPDATE (dn);

  return dn;
}

char *
dn_store_compact (struct dht_node *dn, char *buffer)
{
  memcpy (buffer + 20, &dn->m_sockaddr, 6);
  return buffer + 26;
}

struct dht_object *
dn_store_cache (struct dht_node *dn, struct dht_object *container)
{
  struct string str;

  string_set (&str, "i");
  obj_insert_key_value (container, &str, dn->m_sockaddr.sin_addr.s_addr);

  string_set (&str, "p");
  obj_insert_key_value (container, &str, dn->m_sockaddr.sin_port);

  string_set (&str, "t");
  obj_insert_key_value (container, &str, (signed long) dn->m_lastseen);

  return container;
}
