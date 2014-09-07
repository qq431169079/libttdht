dnl
dnl LibTTDHT - DHT Libaray from libtorrent
dnl
 
AC_PREREQ(2.52)

AC_INIT(libttdht, 0.0.1, naihe2010@126.com)

AC_ARG_ENABLE(debug,
                [  --enable-debug          enable libttdht debug features],
                [libttdht_debug=yes CFLAGS="-D_DEBUG -g $CFLAGS"],
                [CFLAGS="-O2 $CFLAGS"])

AC_ARG_WITH(google-profiler,
            [  --with-google-profiler     with compile google profiler support],
            [with_google_profiler=yes])

AM_CONFIG_HEADER(src/config.h)
AC_CONFIG_AUX_DIR(scripts)
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_LD
AC_PROG_RANLIB
AC_PROG_LIBTOOL
AC_PROG_INSTALL

dnl
dnl Checks for header files.
dnl
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h)
AC_CHECK_FUNC(fcntl)

if test x$with_google_profiler = xyes; then
  AC_CHECK_LIB(profiler, [ProfilerStart, ProfilerStop],
               AC_DEFINE([WITH_PROFILER], 1, [Define to 1 if you have google profiler]) 
               LIBS="-lprofiler $LIBS"
               , AC_MSG_WARN(google profiler library was not found))
fi

# Write generated configuration file
AC_OUTPUT(libttdht.pc
          Makefile
          src/Makefile)
