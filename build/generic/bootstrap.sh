#!/bin/sh
#
# This file builds the configure script and copies all needed files
# provided by automake/libtoolize
#
# $Id: bootstrap.sh,v 1.2 2003-02-20 18:39:23 edgomez Exp $


##############################################################################
# Detect the right autoconf script
##############################################################################

# we first test Debian GNU/Linux autoconf for 2.50 series
AUTOCONF=`which autoconf2.50`

if [ ! -x "$AUTOCONF" ] ; then

    # Test failed, testing generic name -- many distros are shipping 2.13 with
    # that name, that's why we have to perform an additional test for version.
    AUTOCONF=`which autoconf`

    if [ ! -x "$AUTOCONF" ] ; then
	echo "No autoconf binary found in PATH"
	exit -1
    fi

    # Tests the autoconf version
    AC_VER=`$AUTOCONF --version | head -1 | sed 's/^[^0-9]*//i'`
    AC_MAJORVER=`echo $AC_VER | cut -f1 -d'.'`
    AC_MINORVER=`echo $AC_VER | cut -f2 -d'.'`

    if [ "$AC_MAJORVER" -lt "2" ]; then
	echo "This bootstrapper needs Autoconf >= 2.50 (detected $AC_VER)"
	exit -1
    fi

    if [ "$AC_MINORVER" -lt "50" ]; then
	echo "This bootstrapper needs Autoconf >= 2.50 (detected $AC_VER)"
	exit -1
    fi

fi

##############################################################################
# Bootstraps the configure script
##############################################################################

echo "Creating ./configure"
$AUTOCONF

echo "Copying files provided by automake"
automake -c -a 1>/dev/null 2>/dev/null

echo "Copying files provided by libtool"
libtoolize -f -c 1>/dev/null 2>/dev/null

echo "Removing files that are not needed"
rm -rf autom4*
rm -rf ltmain.sh
