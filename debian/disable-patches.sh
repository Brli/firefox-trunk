#!/bin/sh

# Copyright (C) 2010 Canonical Ltd 
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2, or (at
# your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
# Authored by Chris Coulson <chris.coulson@canonical.com>

BASEDIR=$2
DISABLE_PATCHES_LIST=$1

if [ "x" = "x${BASEDIR}" ] ; then
	BASEDIR=`pwd`
fi

if [ "x" = "x${DISABLE_PATCHES_LIST}" ] || [ ! -f ${DISABLE_PATCHES_LIST} ] ; then
	if [ -f ${BASEDIR}/debian/patches/series-disable-patches.${DISABLE_PATCHES_LIST} ] ; then
		DISABLE_PATCHES_LIST="${BASEDIR}/debian/patches/series-disable-patches.${DISABLE_PATCHES_LIST}"
	else
		echo "Must specify a valid list of patches to disable"
		exit 1
	fi
fi

cp ${BASEDIR}/debian/patches/series ${BASEDIR}/debian/patches/series.orig

while read line
do
	cmd="/^"${line}"$/d"
	sed -ri $cmd ${BASEDIR}/debian/patches/series
done < ${DISABLE_PATCHES_LIST}
