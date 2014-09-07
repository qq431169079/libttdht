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
/* @CFILE log.h
*
*  Author: Alf <naihe2010@126.com>
*/
/* @date Created: 2009/07/24 17:01:40 Alf*/

#ifndef _TTDHT_LOG_H_
#define _TTDHT_LOG_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <time.h>

#ifdef WIN32
#define __func__	__FUNCTION__
#endif

#ifdef _DEBUG
#define ttdht_debug(...)      do {                                      \
    printf ("timestamp: %ld\t", time (NULL));                           \
    printf ("DEBUG %s:%d %s():", __FILE__, __LINE__, __func__);         \
    printf (__VA_ARGS__);                                               \
    log_f (0, __VA_ARGS__);                                             \
} while (0)
#else
#define ttdht_debug(...)
#endif

#define ttdht_info(...)       log_f(1, __VA_ARGS__)
#define ttdht_warn(...)       log_f(2, __VA_ARGS__)
#define ttdht_err(...)        log_f(3, __VA_ARGS__)

void log_register (int (*)(int, const char *, void *), void *);

void log_setlevel (int);

void log_f (int loglevel, const char *format, ...);

#endif
