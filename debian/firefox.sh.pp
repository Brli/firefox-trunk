#!/bin/sh

# Firefox launcher containing a Profile migration helper for
# temporary profiles used during alpha and beta phases.

# Authors:
#  Alexander Sack <asac@jwsdot.com>
#  Fabien Tassin <fta@sofaraway.org>
#  Steve Langasek <steve.langasek@canonical.com>
#  Chris Coulson <chris.coulson@canonical.com>
# License: GPLv2 or later

MOZDIR=$HOME/.mozilla
LIBDIR=@DEB_CONFIGURE_LIBEXECDIR@
GDB=/usr/bin/gdb
NAME=`which $0`
APPNAME=`basename $NAME`
MOZ_APP_LAUNCHER=$NAME
SERIES=@SERIES@
BRAND="@STARTUP_WM_CLASS@"
#if DEB_MIN_SYSDEPS == 1
EXE="$APPNAME"-bin
#else
EXE=$APPNAME
#endif

export MOZ_APP_LAUNCHER
unset UBUNTU_MENUPROXY

while [ ! -x $LIBDIR/$EXE ] && [ -L "$NAME" ] ; do
    NAME=`readlink -f $NAME`
    LIBDIR=`dirname $NAME`
done

if [ ! -x $LIBDIR/$EXE ] ; then
	echo "Can't find $LIBDIR/$EXE"
	exit 1
fi

usage () {
    $LIBDIR/$EXE -h | sed -e 's,/.*/,,'
    echo
    echo "      -g or --debug       Start within $GDB (Must be first)"
}

moz_debug=0
moz_debugger_args=""

while [ $# -gt 0 ]; do
    case "$1" in
        -h | --help )
            usage
            exit 0
            ;;
        -g | --debug )
            moz_debug=1
            shift
            ;;
        -a | --debugger-args )
            moz_debugger_args=$2;
            if [ "${moz_debugger_args}" != "" ] ; then
                shift 2
            else
                echo "-a requires an argument"
                exit 1
            fi
            ;;
        -- ) # Stop option processing
            shift
            break
            ;;
        * )
            break
            ;;
    esac
done

if [ -x $LIBDIR/xulapp-profilemigrator ] ; then
    if [ ! -d $MOZDIR/$APPNAME ] ; then
        $LIBDIR/xulapp-profilemigrator -s $SERIES -p $MOZDIR/$APPNAME -b $BRAND
    elif [ -f $MOZDIR/$APPNAME.series-stamp ] ; then
        last_series=`cat $MOZDIR/$APPNAME.series-stamp`
        if [ $SERIES != $last_series ] ; then
            $LIBDIR/xulapp-profilemigrator -s $SERIES -p $MOZDIR/$APPNAME -b $BRAND
        fi
    else
	$LIBDIR/xulapp-profilemigrator -s $SERIES -p $MOZDIR/$APPNAME -b $BRAND
    fi
fi

#if DEB_MIN_SYSDEPS == 1
LD_LIBRARY_PATH=${LIBDIR}:${LIBDIR}/plugins${LD_LIBRARY_PATH:+":$LD_LIBRARY_PATH"}
export LD_LIBRARY_PATH
#endif

if [ $moz_debug -eq 1 ] ; then
    exec $GDB $moz_debugger_args --args $LIBDIR/$EXE "$@"
else
    exec $LIBDIR/$EXE "$@"
fi
