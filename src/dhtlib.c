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
/* @CFILE dhtlib.c
*
*  Author: Alf <naihe2010@126.com>
*/
/* @date Created: 2009/08/19 11:16:40 Alf*/

#include "dhtlog.h"
#include "dhtlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <openssl/sha.h>

static int buf_to_object_read_value (signed long *, struct string *);
static int buf_to_object_read_str (struct string *, struct string *);
static int object_to_buf_write_char (struct string *, char);
static int object_to_buf_write_value (struct string *, signed long);
static int object_to_buf_write_string (struct string *, const char *, int);

void
hashsg_init (const char *src, unsigned int len, char *buffer)
{
  SHA_CTX ctx[1];

  SHA1_Init (ctx);
  SHA1_Update (ctx, src, len);
  SHA1_Final ((unsigned char *) buffer, ctx);
}

void
hashsg_clear (char *buffer, int v)
{
  memset (buffer, v, HASH_STRING_LEN);
}

int
hashsg_cmp (const char *s1, const char *s2)
{
  return memcmp (s1, s2, HASH_STRING_LEN);
}

void
hashsg_cpy (char *s1, const char *s2)
{
  memcpy (s1, s2, HASH_STRING_LEN);
  s1[HASH_STRING_LEN] = 0;
}

int
hashsg_closer (const char *target, const char *s1, const char *s2)
{
  unsigned int i;

  for (i = 0; i < HASH_STRING_LEN; i++)
    {
      if (s1[i] != s2[i])
	{
	  return (unsigned char) (s1[i] ^ target[i]) <
	    (unsigned char) (s2[i] ^ target[i]);
	}
    }

  return 0;
}

struct string *
string_init (char *data, int len)
{
  return NULL;
}

struct string *
string_set (struct string *str, const char *data)
{
  return string_set2 (str, data, (int) strlen (data));
}

struct string *
string_set2 (struct string *str, const char *data, int len)
{
  str->data = (char *) data;
  str->len = len;
  return str;
}

void
string_ext (struct string *str, int len)
{
  if (str->data == NULL)
    str->data = (char *) calloc (len, sizeof (char));
  else
    str->data = (char *) realloc (str->data, len * sizeof (char));

  assert (str->data);
}

char *
string_cpy (struct string *dst, struct string *src)
{
  if (dst->len <= src->len)
    {
      string_ext (dst, src->len + 1);
    }

  memcpy (dst->data, src->data, src->len);
  dst->data[src->len] = '\0';

  dst->len = src->len;

  return dst->data;
}

int
string_cmp (struct string *s1, struct string *s2)
{
  return (s1->len == s2->len) ? memcmp (s1->data, s2->data,
					s1->len) : (s1->len - s2->len);
}

void
string_clear (struct string *str)
{
  if (str->data != NULL)
    {
      free (str->data);
      str->data = NULL;
    }

  str->len = 0;
}

struct list_node *
list_node_init (void *arg)
{
  struct list_node *item;

  item = (struct list_node *) calloc (1, sizeof (struct list_node));
  assert (item);
  item->item = arg;

  return item;
}

void
list_node_cleanup (struct list_node *node)
{
  free (node);
}

struct list *
list_init (void)
{
  struct list *list;

  list = (struct list *) calloc (1, sizeof (struct list));
  assert (list);

  LIST_INIT (list);

  return list;
}

void
list_insert_head (struct list *list, void *arg)
{
  struct list_node *item = list_node_init (arg);
  assert (item);

  LIST_INSERT_HEAD (list, item, entries);
  list->size++;
}

void
list_insert_after (struct list *list, struct list_node *node, void *arg)
{
  struct list_node *item = list_node_init (arg);
  assert (item);

  LIST_INSERT_AFTER (node, item, entries);
  list->size++;
}

void
list_insert_before (struct list *list, struct list_node *node, void *arg)
{
  struct list_node *item = list_node_init (arg);
  assert (item);

  LIST_INSERT_BEFORE (node, item, entries);
  list->size++;
}

void *
list_first (struct list *list)
{
  return LIST_FIRST (list)->item;
}

void *
list_end (struct list *list)
{
  return NULL;
}

int
list_empty (struct list *list)
{
  return list->size == 0;
}

void
list_remove (struct list *list, struct list_node *item)
{
  list->size--;
  LIST_REMOVE (item, entries);
  free (item);
}

struct map_node *
map_node_init (struct string *key, void *sec)
{
  struct map_node *node;

  node = (struct map_node *) calloc (1, sizeof (struct map_node));
  assert (node);

  string_cpy (&node->key, key);

  node->value = sec;

  return node;
}

void
map_node_cleanup (struct map_node *node)
{
  string_clear (&node->key);
  free (node);
}

struct map *
map_init (void)
{
  struct map *map;

  map = (struct map *) calloc (1, sizeof (struct map));
  assert (map);

  LIST_INIT (map);

  return map;
}

struct map_node *
map_find (struct map *map, struct string *key)
{
  struct map_node *mn;

  LIST_FOREACH (mn, map, entries)
  {
    if (string_cmp (key, &mn->key) == 0)
      return mn;
  }

  return NULL;
}

void
map_insert_head (struct map *map, struct string *key, void *arg)
{
  struct map_node *item = map_node_init (key, arg);
  assert (item);

  LIST_INSERT_HEAD (map, item, entries);
  map->size++;
}

void
map_insert_after (struct map *map, struct map_node *node, struct string *key,
		  void *arg)
{
  struct map_node *item = map_node_init (key, arg);
  assert (item);

  LIST_INSERT_AFTER (node, item, entries);
  map->size++;
}

void
map_insert_before (struct map *map, struct map_node *node, struct string *key,
		   void *arg)
{
  struct map_node *item = map_node_init (key, arg);
  assert (item);

  LIST_INSERT_BEFORE (node, item, entries);
  map->size++;
}

void
map_remove (struct map *map, struct map_node *node)
{
  map->size--;
  LIST_REMOVE (node, entries);
  map_node_cleanup (node);
}

void
map_clear (struct map *map)
{
  struct map_node *mn;

  while ((mn = map_begin (map)) != NULL)
    {
      map_remove (map, mn);
    }
}

int
map_empty (struct map *map)
{
  return map->size == 0;
}

struct map_node *
map_begin (struct map *map)
{
  return LIST_FIRST (map);
}

struct map_node *
map_end (struct map *map)
{
  return NULL;
}

struct dht_object *
obj_init (obj_type type)
{
  struct dht_object *obj;

  obj = (struct dht_object *) calloc (1, sizeof (struct dht_object));
  assert (obj);

  OBJ_TYPE (obj) = type;

  switch (type)
    {
    case OBJ_TYPE_LIST:
      LIST_INIT (&obj->m_list);
      break;
    case OBJ_TYPE_MAP:
      LIST_INIT (&obj->m_map);
      break;
    default:
      break;
    }

  return obj;
}

void
obj_cleanup (struct dht_object *obj)
{
  struct list_node *ln;
  struct map_node *mn;

  switch (OBJ_TYPE (obj))
    {
    case OBJ_TYPE_NONE:
    case OBJ_TYPE_VALUE:
      break;

    case OBJ_TYPE_STRING:
      string_clear (&obj->m_string);
      break;

    case OBJ_TYPE_LIST:
      while ((ln = LIST_FIRST (&obj->m_list)) != NULL)
	{
	  if (ln->item)
	    obj_cleanup ((struct dht_object *) ln->item);

	  list_remove (&obj->m_list, ln);
	}
      break;

    case OBJ_TYPE_MAP:
      while ((mn = map_begin (&obj->m_map)) != NULL)
	{
	  if (mn->value)
	    obj_cleanup ((struct dht_object *) mn->value);

	  map_remove (&obj->m_map, mn);
	}
      break;

    default:
      break;
    }

  free (obj);
}

int
obj_has_key (struct dht_object *obj, struct string *key)
{
  struct map_node *mn;

  mn = map_find (&obj->m_map, key);
  return (mn == NULL) ? 0 : 1;
}

int
obj_has_key_string (struct dht_object *obj, struct string *key)
{
  struct map_node *mn;

  mn = map_find (&obj->m_map, key);
  if (mn != NULL)
    {
      if (OBJ_TYPE ((struct dht_object *) mn->value) == OBJ_TYPE_STRING)
	return 1;
    }

  return 0;
}

int
obj_has_key_value (struct dht_object *obj, struct string *key)
{
  struct map_node *mn;

  mn = map_find (&obj->m_map, key);
  if (mn != NULL)
    {
      if (OBJ_TYPE ((struct dht_object *) mn->value) == OBJ_TYPE_VALUE)
	return 1;
    }

  return 0;
}

int
obj_has_key_list (struct dht_object *obj, struct string *key)
{
  struct map_node *mn;

  mn = map_find (&obj->m_map, key);
  if (mn != NULL)
    {
      if (OBJ_TYPE ((struct dht_object *) mn->value) == OBJ_TYPE_LIST)
	return 1;
    }

  return 0;
}

int
obj_has_key_map (struct dht_object *obj, struct string *key)
{
  struct map_node *mn;

  mn = map_find (&obj->m_map, key);
  if (mn != NULL)
    {
      if (OBJ_TYPE ((struct dht_object *) mn->value) == OBJ_TYPE_MAP)
	return 1;
    }

  return 0;
}

struct dht_object *
obj_get_key (struct dht_object *obj, struct string *key)
{
  struct map_node *mn;

  mn = map_find (&obj->m_map, key);
  return (mn == NULL) ? NULL : (struct dht_object *) mn->value;
}

struct string *
obj_get_key_string (struct dht_object *obj, struct string *key)
{
  struct map_node *mn;

  mn = map_find (&obj->m_map, key);
  if (mn != NULL)
    {
      if (OBJ_TYPE ((struct dht_object *) mn->value) == OBJ_TYPE_STRING)
	{
	  return &((struct dht_object *) mn->value)->m_string;
	}
    }

  return NULL;
}

signed long
obj_get_key_value (struct dht_object *obj, struct string *key)
{
  struct map_node *mn;

  mn = map_find (&obj->m_map, key);
  if (mn != NULL)
    {
      if (OBJ_TYPE ((struct dht_object *) mn->value) == OBJ_TYPE_VALUE)
	return ((struct dht_object *) mn->value)->m_value;
    }

  return 0;
}

struct list *
obj_get_key_list (struct dht_object *obj, struct string *key)
{
  struct map_node *mn;

  mn = map_find (&obj->m_map, key);
  if (mn != NULL)
    {
      if (OBJ_TYPE ((struct dht_object *) mn->value) == OBJ_TYPE_LIST)
	return &((struct dht_object *) mn->value)->m_list;
    }

  return NULL;
}

struct map *
obj_get_key_map (struct dht_object *obj, struct string *key)
{
  struct map_node *mn;

  mn = map_find (&obj->m_map, key);
  if (mn != NULL)
    {
      if (OBJ_TYPE ((struct dht_object *) mn->value) == OBJ_TYPE_MAP)
	return &((struct dht_object *) mn->value)->m_map;
    }

  return NULL;
}

struct dht_object *
obj_insert_key_object (struct dht_object *obj, struct string *key,
		       struct dht_object *obi)
{
  map_insert_head (&obj->m_map, key, obi);
  return obi;
}

struct dht_object *
obj_insert_key_string (struct dht_object *obj, struct string *key,
		       struct string *value)
{
  struct dht_object *obi;

  obi = obj_init (OBJ_TYPE_STRING);
  string_cpy (&obi->m_string, value);

  return obj_insert_key_object (obj, key, obi);
}

struct dht_object *
obj_insert_key_value (struct dht_object *obj, struct string *key,
		      signed long value)
{
  struct dht_object *obi;

  obi = obj_init (OBJ_TYPE_VALUE);
  obi->m_value = value;

  return obj_insert_key_object (obj, key, obi);
}

struct dht_object *
obj_insert_list_object (struct dht_object *obj, struct dht_object *obi)
{
  list_insert_head (&obj->m_list, obi);
  return obi;
}

struct dht_object *
obj_insert_list_string (struct dht_object *obj, struct string *str)
{
  struct dht_object *obi;

  obi = obj_init (OBJ_TYPE_STRING);
  string_cpy (&obi->m_string, str);

  list_insert_head (&obj->m_list, obi);

  return obi;
}

struct dht_object *
obj_insert_list_value (struct dht_object *obj, signed long value)
{
  struct dht_object *obi;

  obi = obj_init (OBJ_TYPE_VALUE);
  obi->m_value = value;

  list_insert_head (&obj->m_list, obi);

  return obi;
}

static int
object_to_buf_write_value (struct string *buf, signed long sr)
{
  char temp[20] = { 0 }, *p;
  int ret;

  if (sr == 0)
    return object_to_buf_write_char (buf, '0');

  if (sr < 0)
    {
      ret = object_to_buf_write_char (buf, '-');
      if (ret < 0)
	return -1;
      sr = -sr;
    }

  p = temp + 20;
  while (sr != 0)
    {
      *(--p) = (char) ('0' + sr % 10);
      sr /= 10;
    }

  return object_to_buf_write_string (buf, p, (int) (20 - (p - temp)));
}

static int
object_to_buf_write_char (struct string *buf, char src)
{
  if (buf->len < 1)
    return -1;

  *(buf->data) = src;
  string_step (buf, 1);
  return 0;
}

static int
object_to_buf_write_string (struct string *buf, const char *src, int len)
{
  if (buf->len < len)
    return -1;

  memcpy (buf->data, src, len);
  string_step (buf, len);
  return 0;
}

int
object_to_buf (struct dht_object *obj, struct string *buf)
{
  struct list_node *ln;
  struct map_node *mn;
  struct string *str;
  int ret;

  switch (OBJ_TYPE (obj))
    {
    case OBJ_TYPE_NONE:
      break;

    case OBJ_TYPE_VALUE:
      ret = object_to_buf_write_char (buf, 'i');
      if (ret < 0)
	return -1;
      ret = object_to_buf_write_value (buf, OBJ_AS_VALUE (obj));
      if (ret < 0)
	return -1;
      ret = object_to_buf_write_char (buf, 'e');
      if (ret < 0)
	return -1;
      break;

    case OBJ_TYPE_STRING:
      str = OBJ_AS_STRING (obj);
      ret = object_to_buf_write_value (buf, str->len);
      if (ret < 0)
	return -1;
      ret = object_to_buf_write_char (buf, ':');
      if (ret < 0)
	return -1;
      ret = object_to_buf_write_string (buf, str->data, str->len);
      if (ret < 0)
	return -1;
      break;

    case OBJ_TYPE_LIST:
      ret = object_to_buf_write_char (buf, 'l');
      if (ret < 0)
	return -1;

      LIST_FOREACH (ln, &obj->m_list, entries)
      {
	ret = object_to_buf ((struct dht_object *) ln->item, buf);
	if (ret < 0)
	  return -1;
      }

      ret = object_to_buf_write_char (buf, 'e');
      if (ret < 0)
	return -1;
      break;

    case OBJ_TYPE_MAP:
      ret = object_to_buf_write_char (buf, 'd');
      if (ret < 0)
	return -1;

      LIST_FOREACH (mn, &obj->m_map, entries)
      {
	ret = object_to_buf_write_value (buf, mn->key.len);
	if (ret < 0)
	  return -1;
	ret = object_to_buf_write_char (buf, ':');
	if (ret < 0)
	  return -1;
	ret = object_to_buf_write_string (buf, mn->key.data, mn->key.len);
	if (ret < 0)
	  return -1;

	ret = object_to_buf ((struct dht_object *) mn->value, buf);
	if (ret < 0)
	  return -1;
      }

      ret = object_to_buf_write_char (buf, 'e');
      if (ret < 0)
	return -1;
      break;
    }

  return 0;
}

struct dht_object *
buf_to_object (struct string *buf)
{
  struct dht_object *ob, *ob2;
  struct string key;
  char *errstr;
  int ret;

  ob = NULL;
  errstr = NULL;

  switch (*(buf->data))
    {
    case 'i':
      string_step (buf, 1);
      if (buf->len < 0)
	return NULL;

      ob = obj_init (OBJ_TYPE_VALUE);
      assert (ob);

      ret = buf_to_object_read_value (&ob->m_value, buf);

      if (ret < 0)
	{
	  errstr = "Read value from buffer error.";
	}

      break;

    case 'l':
      string_step (buf, 1);
      if (buf->len < 0)
	return NULL;

      ob = obj_init (OBJ_TYPE_LIST);
      assert (ob);

      while (buf->len)
	{
	  if (*(buf->data) == 'e')
	    {
	      string_step (buf, 1);
	      break;
	    }

	  ob2 = buf_to_object (buf);
	  if (ob2 != NULL)
	    {
	      list_insert_head (&ob->m_list, ob2);
	    }
	}

      break;

    case 'd':
      string_step (buf, 1);
      if (buf->len < 0)
	return NULL;

      ob = obj_init (OBJ_TYPE_MAP);
      assert (ob);

      while (buf->len)
	{
	  if (*(buf->data) == 'e')
	    {
	      string_step (buf, 1);
	      break;
	    }

	  memset (&key, 0, sizeof (key));
	  ret = buf_to_object_read_str (&key, buf);
	  if (ret < 0)
	    {
	      errstr = "Read map node key error.";
	      break;
	    }

	  ob2 = buf_to_object (buf);
	  if (ob2 == NULL)
	    {
	      errstr = "Read map node value error";
	      break;
	    }

	  if (key.len && ob2)
	    {
	      map_insert_head (&ob->m_map, &key, ob2);
	    }
	  string_clear (&key);
	}

      break;

    default:
      if (*(buf->data) >= '0' && *(buf->data) <= '9')
	{
	  ob = obj_init (OBJ_TYPE_STRING);
	  assert (ob);

	  ret = buf_to_object_read_str (&ob->m_string, buf);
	  if (ret < 0)
	    {
	      errstr = "Read str from buffer error.";
	    }
	}

      break;
    }

  if (errstr != NULL)
    {
      ttdht_err ("%s\n", errstr);
      obj_cleanup (ob);
      ob = NULL;
    }

  return ob;
}

int
buf_to_object_read_value (signed long *val, struct string *buf)
{
  char *m, temp[20];
  int size;

  m = strchr (buf->data, 'e');
  if (m == NULL)
    return -1;

  size = (unsigned int) (m - buf->data);
  if (size > buf->len || size > 19)
    return -1;

  memcpy (temp, buf->data, size);
  temp[size] = '\0';

  *val = atol (temp);

  string_step (buf, 1 + size);

  return 0;
}

int
buf_to_object_read_str (struct string *str, struct string *buf)
{
  struct string str2;
  char *m, temp[8];
  int size, len;

  m = strchr (buf->data, ':');
  if (m == NULL)
    return -1;

  size = (unsigned int) (m - buf->data);
  if (size + 1 > buf->len || size > 7)
    return -1;

  memcpy (temp, buf->data, size);
  temp[size] = '\0';

  len = atoi (temp);
  if (len > buf->len - size)
    return -1;

  string_set2 (&str2, m + 1, len);
  string_cpy (str, &str2);

  string_step (buf, size + 1 + len);
  return 0;
}
