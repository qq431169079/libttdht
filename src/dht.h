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
/* @CFILE dht.h
*
*  Author: Alf <naihe2010@126.com>
*/
/* @date Created: 2009/08/19 11:16:40 Alf*/

#ifndef _DHT_H_
#define _DHT_H_

#include "dhtrouter.h"

typedef struct
{
  struct dht_router *router;
  char *inifile;
  void *valid_timer;
} dht_t;

dht_t *dht_new (const char *, int, dhtio_t *);

void dht_delete (dht_t *);

void dht_add_friend (dht_t *, const char *, unsigned short);

void dht_pub_keyword (dht_t *, const char *, int);

void dht_get_keyword (dht_t *, const char *,
		      void (*)(const char *, const char *, void *), void *);

#endif
