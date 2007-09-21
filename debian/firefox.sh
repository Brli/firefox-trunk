#!/bin/sh

# Script written by Fabien Tassin <fta@sofaraway.org> in September 2007

# If there's still no ~/mozilla/firefox-3.0 profile, try to find a previous
# firefox profile and initialize with that. We'll try to first use
# 'firefox-trunk' profile, then try 'grandparadiso', then # plain 'firefox'.
# If nothing is found, we'll go for a fresh run and let firefox create a
# default profile for us.

APPNAME=firefox-3.0
MOZDIR=$HOME/.mozilla

if [ ! -d $MOZDIR/$APPNAME ] ; then
  FOUND=""
  if [ -d $MOZDIR/firefox-trunk ] ; then
    FOUND=firefox-trunk
  elif [ -d $MOZDIR/granparadiso ] ; then
    FOUND=granparadiso
  elif [ -�d $MOZDIR/firefox ] ; then
    FOUND=firefox
  fi

  if [ "$FOUND" != "" ] ; then
    echo "*NOTICE* No previous $APPNAME profile found, we'll initialize a profile using a copy of your existing '$FOUND' profile."
    echo -n "Transfering..."
    cp -a $MOZDIR/$FOUND $MOZDIR/$APPNAME
    echo " done."
  else
    echo "*NOTICE* No previous firefox profile found, starting with a fresh one"
  fi
fi

exec /usr/lib/$APPNAME/$APPNAME "$@"
