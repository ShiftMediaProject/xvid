#!/bin/sh
#
# This file builds the configure script and copies all needed files
# provided by automake/libtoolize
#
# NB: This script is adapted to Debian GNU/Linux SID program names
#     Perhaps you could have to modify program names to match your distro

echo "Creating ./configure"
autoconf2.50

echo "Copying files provided by automake"
automake -c -a 1>/dev/null 2>/dev/null

echo "Copying files provided by libtool"
libtoolize -f -c 1>/dev/null 2>/dev/null

echo "Removing files that are not needed"
rm -rf autom4*
rm -rf ltmain.sh
