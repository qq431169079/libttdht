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
/* @CFILE dhttest.c
*
*  Author: Alf <naihe2010@126.com>
*/
/* @date Created: 2009/08/19 11:16:40 Alf*/

#include "dht.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

static int dht_fd;
static ssize_t dht_read (void *, char *, size_t, struct sockaddr_in *, int);
static ssize_t dht_write (void *, const char *, size_t, struct sockaddr_in *);

int
main (int argc, char *argv[])
{
  struct sockaddr_in addr[1];
  dhtio_t io[1] = { {
      &dht_fd,
        dht_read,
        dht_write
  }};
  dht_t *dht;

  dht_fd = socket (AF_INET, SOCK_DGRAM, 0);
  assert (dht_fd > 0);

  addr->sin_family = AF_INET;
  addr->sin_addr.s_addr = inet_addr ("0.0.0.0");
  addr->sin_port = htons (6681);
  memset (addr->sin_zero, 0, 8);

  assert (!bind (dht_fd, (struct sockaddr *) addr, sizeof (struct sockaddr)));

  dht = dht_new ("dht.cache", 6681, io);
  return 0;
}

static ssize_t 
dht_read (void *p, char *buf, size_t size, struct sockaddr_in *sa, int tim)
{
  int fd;
  ssize_t ret;
  socklen_t siz;
  fd_set set[1];
  struct timeval tv[1];

  fd = * (int *) p;

  FD_ZERO (set);
  FD_SET (fd, set);
  tv->tv_sec = tim;
  tv->tv_usec = 0;
  if (select (fd + 1, set, NULL, NULL, tv) <= 0
      || !FD_ISSET (fd, set))
    {
      return -1;
    }

  siz = sizeof (struct sockaddr);
  ret = recvfrom (fd, buf, size, 0, (struct sockaddr *) sa, &siz);

  return ret;
}

static ssize_t 
dht_write (void *p, const char *buf, size_t size, struct sockaddr_in *sa)
{
  int fd;
  ssize_t ret;
  socklen_t siz;

  fd = * (int *) p;
  siz = sizeof (struct sockaddr);
  ret = sendto (fd, buf, size, 0, (struct sockaddr *) sa, siz);

  return ret;
}
