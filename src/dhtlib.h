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
/* @CFILE dhtlib.h
*
*  Author: Alf <naihe2010@126.com>
*/
/* @date Created: 2009/08/19 11:16:40 Alf*/

#ifndef _DHT_LIB_H_
#define _DHT_LIB_H_

#include "queue.h"

#define HASH_STRING_LEN         20
#define LOCAL_TOKEN_LEN         4
#define TOKEN_LEN               20

#define string_step(b, i)       do { (b)->data += (i); (b)->len -= (i); } while (0)

struct string
{
  char *data;
  int len;
};

struct string *string_init (char *, int);

struct string *string_set (struct string *, const char *);

struct string *string_set2 (struct string *, const char *, int);

void string_ext (struct string *, int);

char *string_cpy (struct string *, struct string *);

int string_cmp (struct string *, struct string *);

void string_clear (struct string *);

void string_cleanup (struct string *);

struct list_node
{
  void *item;
    LIST_ENTRY (list_node) entries;
};

struct list
{
  struct list_node *lh_first;
  unsigned int size;
};

struct list_node *list_node_init (void *);
void list_node_cleanup (struct list_node *);

struct list *list_init (void);
void list_insert_head (struct list *, void *);
void list_insert_after (struct list *, struct list_node *, void *);
void list_insert_before (struct list *, struct list_node *, void *);
void *list_first (struct list *);
void *list_end (struct list *);
int list_empty (struct list *);
void list_remove (struct list *, struct list_node *);

struct map_node
{
  struct string key;
  const void *value;
    LIST_ENTRY (map_node) entries;
};

#define MAP_SIZE(map)   ((map)->size)
#define MAP_EMPTY(map)   ((map)->size == 0)

struct map
{
  struct map_node *lh_first;
  unsigned int size;
};

struct map_node *map_node_init (struct string *, void *);

struct map *map_init (void);
void map_cleanup (struct map_node *);
struct map_node *map_find (struct map *, struct string *);
void map_insert_head (struct map *, struct string *, void *);
void map_insert_after (struct map *, struct map_node *, struct string *,
		       void *);
void map_insert_before (struct map *, struct map_node *, struct string *,
			void *);
void map_remove (struct map *, struct map_node *);
void map_clear (struct map *);
int map_empty (struct map *);
struct map_node *map_begin (struct map *);
struct map_node *map_end (struct map *);

#define OBJ_TYPE(obj)                   ((obj)->type)

#define  OBJ_AS_VALUE(obj)              (OBJ_TYPE (obj) != OBJ_TYPE_VALUE? 0: (obj)->m_value)
#define  OBJ_AS_STRING(obj)             (OBJ_TYPE (obj) != OBJ_TYPE_STRING? NULL: &(obj)->m_string)
#define  OBJ_AS_LIST(obj)               (OBJ_TYPE (obj) != OBJ_TYPE_LIST? NULL: &(obj)->m_list)
#define  OBJ_AS_MAP(obj)                (OBJ_TYPE (obj) != OBJ_TYPE_MAP? NULL: &(obj)->m_map)

#define  OBJ_IS_VALUE(obj)              (OBJ_TYPE (obj) == OBJ_TYPE_VALUE)
#define  OBJ_IS_STRING(obj)             (OBJ_TYPE (obj) == OBJ_TYPE_STRING)
#define  OBJ_IS_LIST(obj)               (OBJ_TYPE (obj) == OBJ_TYPE_LIST)
#define  OBJ_IS_MAP(obj)                (OBJ_TYPE (obj) == OBJ_TYPE_MAP)

typedef enum
{
  OBJ_TYPE_NONE,
  OBJ_TYPE_VALUE,
  OBJ_TYPE_STRING,
  OBJ_TYPE_LIST,
  OBJ_TYPE_MAP,
} obj_type;

struct dht_object
{
  obj_type type;
  union
  {
    signed long m_value;
    struct string m_string;
    struct list m_list;
    struct map m_map;
  };
};

struct dht_object *obj_init (obj_type);

void obj_cleanup (struct dht_object *);

int obj_has_key (struct dht_object *, struct string *);

int obj_has_key_string (struct dht_object *, struct string *);

int obj_has_key_value (struct dht_object *, struct string *);

int obj_has_key_list (struct dht_object *, struct string *);

int obj_has_key_map (struct dht_object *, struct string *);

struct dht_object *obj_get_key (struct dht_object *, struct string *);

struct string *obj_get_key_string (struct dht_object *, struct string *);

signed long obj_get_key_value (struct dht_object *, struct string *);

struct list *obj_get_key_list (struct dht_object *, struct string *);

struct map *obj_get_key_map (struct dht_object *, struct string *);

struct dht_object *obj_insert_key_object (struct dht_object *,
					  struct string *,
					  struct dht_object *);

struct dht_object *obj_insert_key_string (struct dht_object *,
					  struct string *, struct string *);

struct dht_object *obj_insert_key_value (struct dht_object *,
					 struct string *, signed long);

struct dht_object *obj_insert_list_object (struct dht_object *,
					   struct dht_object *);

struct dht_object *obj_insert_list_value (struct dht_object *obj,
					  signed long);

struct dht_object *obj_insert_list_string (struct dht_object *,
					   struct string *);

int object_to_buf (struct dht_object *, struct string *);

struct dht_object *buf_to_object (struct string *);

void hashsg_init (const char *, unsigned int, char *);

void hashsg_clear (char *, int);

int hashsg_cmp (const char *, const char *);

void hashsg_cpy (char *, const char *);

int hashsg_closer (const char *, const char *, const char *);

#endif
