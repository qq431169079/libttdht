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
/* @CFILE dht.c
*
*  Author: Alf <naihe2010@126.com>
*/
/* @date Created: 2009/08/19 11:16:40 Alf*/

#include "dhtlog.h"
#include "dhtbucket.h"
#include "dhtnode.h"
#include "dhtrouter.h"
#include "dhtserver.h"
#include "dhttrans.h"
#include "dht.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

dht_t *
dht_new (const char *inifile, int port, dhtio_t *io)
{
  struct dht_object *cache;
  dht_t *du;
  FILE *fp;
  char buf[0x2000];
  int ret;

  if (io->read == NULL
      || io->write == NULL)
    {
      ttdht_err ("IO error.\n");
      return NULL;
    }

  du = (dht_t *) calloc (1, sizeof (dht_t));
  if (du == NULL)
    {
      ttdht_err ("Memroy error.\n");
      return NULL;
    }

  cache = NULL;
  du->inifile = strdup (inifile);
  if ((du->inifile != NULL) && (fp = fopen (du->inifile, "rb")) != NULL)
    {
      struct string str;
      int total;

      total = fread (buf, 1, sizeof buf, fp);
      string_set2 (&str, buf, total);
      cache = buf_to_object (&str);
    }

  du->router = dr_init (cache, port, io);

  if (cache)
    obj_cleanup (cache);

  ret = dr_start (du->router, port);
  if (ret < 0)
    {
      dr_cleanup (du->router);
      free (du);
      return NULL;
    }

  dr_add_contact (du->router, "60.21.161.146", 6501);
  dr_add_contact (du->router, "187.13.201.118", 12785);
  dr_add_contact (du->router, "221.211.182.70", 15129);
  dr_add_contact (du->router, "71.171.15.2", 6881);
  dr_add_contact (du->router, "71.79.19.133", 27186);
  dr_add_contact (du->router, "84.3.116.151", 21648);
  dr_add_contact (du->router, "142.161.176.152", 44366);
  dr_add_contact (du->router, "88.193.85.9", 6881);

  dr_run (du->router);

  return du;
}

void
dht_delete (dht_t * du)
{
  struct dht_object *obj;

  obj = NULL;

  if (du->inifile != NULL)
    {
      FILE *fp;
      char temp[8192];
      unsigned int len;
      struct string str;
      int ret;

      obj = obj_init (OBJ_TYPE_MAP);
      if (obj == NULL)
	{
	  ttdht_err ("create dht object error.\n");
	  goto ret;
	}

      dr_store_cache (du->router, obj);

      fp = fopen (du->inifile, "wb");
      if (fp == NULL)
	{
	  ttdht_err ("Open bootstrap ini file error.\n");
	  goto ret;
	}

      string_set2 (&str, temp, sizeof temp);
      ret = object_to_buf (obj, &str);

      if (ret < 0)
	{
	  ttdht_err ("Create dht object to buffer error.\n");
	  fclose (fp);
	  goto ret;
	}

      len = str.data - temp;

      fwrite (temp, len, 1, fp);
      fclose (fp);
    }

ret:
  if (du->inifile != NULL)
    {
      free (du->inifile);
    }

  if (obj != NULL)
    {
      obj_cleanup (obj);
    }

  dr_stop (du->router);

  dr_cleanup (du->router);
  free (du);
}

void
dht_add_friend (dht_t * dht, const char *ip, unsigned short port)
{
  dr_add_contact (dht->router, ip, port);
}

void
dht_pub_keyword (dht_t * du, const char *key, int serv_port)
{
  dr_pub (du->router, key, serv_port);
}

void
dht_get_keyword (dht_t * du, const char *key,
		 void (*cb) (const char *, const char *, void *), void *arg)
{
  dr_get (du->router, key, cb, arg);
}
