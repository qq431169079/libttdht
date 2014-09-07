#!/bin/sh

echo "Generating configuration files for libttdht, please wait..."

echo "  libtoolize ..."
libtoolize --automake --force --copy

echo "  aclocal ..."
aclocal

echo "  autoheader ..."
autoheader

echo "  automake ..."
automake -a -c

echo "  autoconf ..."
autoconf
