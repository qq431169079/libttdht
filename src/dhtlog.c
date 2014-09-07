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
/* @CFILE log.c
*
*  Author: Alf <naihe2010@126.com>
*/
/* @date Created: 2009/07/24 17:01:40 Alf*/

#include "dhtlog.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

static int print_level = 1;
static int (*print_func) (int, const char *, void *) = NULL;
static void *print_arg;

void
log_register (int (*cb) (int, const char *, void *), void *arg)
{
  print_func = cb;
  print_arg = arg;
}

void
log_setlevel (int level)
{
  print_level = level;
}

static const char *level_phrase[] = {
    "DEBUG: ",
    "INFO: ",
    "WARNING: ",
    "ERROR: "
};

void
log_f (int loglevel, const char *format, ...)
{
  va_list ap;
  char buf[1024];

  if (loglevel < print_level)
    {
      return;
    }

  va_start (ap, format);

  if (print_func)
    {
      vsnprintf (buf, sizeof buf, format, ap);
      print_func (loglevel, buf, print_arg);
    }
  else
    {
      if (loglevel >= 0 && loglevel < sizeof (level_phrase) / sizeof
          (level_phrase[0]))
        {
          printf ("%s: ", level_phrase[loglevel]);
        }
      vprintf (format, ap);
    }

  va_end (ap);
}
