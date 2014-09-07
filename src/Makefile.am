includedir = $(PREFIX)/include/libttdht

AM_CFLAGS = -Wall

include_HEADERS = \
		  queue.h \
                  dhtbucket.h \
                  dht.h \
                  dhtlib.h \
                  dhtnode.h \
                  dhtrouter.h \
                  dhtserver.h \
                  dhttracker.h \
                  dhttrans.h \
                  dhtlog.h

libttdht_la_SOURCES = \
                      dhtbucket.c \
                      dht.c \
                      dhtlib.c \
                      dhtnode.c \
                      dhtrouter.c \
                      dhtserver.c \
                      dhttracker.c \
                      dhttrans.c \
                      dhtlog.c

lib_LTLIBRARIES = libttdht.la

check_PROGRAMS = dhttest

dhttest_SOURCES = dhttest.c
dhttest_LDADD = -lssl .libs/libttdht.a
