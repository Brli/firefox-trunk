# -*- mode: makefile; coding: utf-8 -*-

# Copyright (c) 2008 Fabien Tassin <fta@sofaraway.org>
# Description: Project firefox 3.1
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

include /usr/share/mozilla-devscripts/mozclient.mk

COMPARE_FILTER_PRE_IN := sed \
	-e 's,foo,bar,' \
	$(NULL)

COMPARE_FILTER_PRE_OUT := sed \
	-e 's,^usr/lib/firefox-3.1[^/]*/,,' \
        $(NULL)

COMPARE_FILTER_IN    := sed \
	-e 's,^usr/lib/firefox-3.1[^/]*/searchplugins,usr/lib/firefox-addons/searchplugins,' \
	-e 's,^usr/lib/firefox-3.1[^/]*/extensions/inspector@mozilla.org/.*,,' \
	-e 's,^usr/lib/firefox-3.1[^/]*/extensions,usr/lib/xulrunner-addons/extensions,' \
	-e 's,^usr/lib/firefox-3.1[^/]*/defaults/profile,etc/firefox-3.1/profile,' \
	-e 's,^usr/lib/firefox-3.1[^/]*/\(old-homepage-default.properties\|README.txt\|removed-files\),,' \
	-e 's,^usr/lib/firefox-3.1[^/]*/.autoreg,,' \
	-e 's,^etc/firefox-3.1[^/]*/.autoreg,,' \
	$(NULL)

COMPARE_FILTER_OUT   := sed \
	-e 's,^DEBIAN/.*,,' \
	-e 's,^usr/lib/debug/.*,,' \
	-e 's,^usr/share/doc/.*,,' \
	-e 's,^usr/share/menu/.*,,' \
	-e 's,^usr/share/applications/.*,,' \
	-e 's,^usr/share/bug/firefox-3.1/presubj,,' \
	-e 's,^etc/firefox-3.1/\(firefoxrc\|pref/firefox.js\),,' \
	-e 's,^usr/lib/firefox-addons/searchplugins/\(debsearch\|wikipedia\).\(gif\|src\),,' \
	-e 's,^usr/lib/firefox-3.1[^/]*/\(firefox-3.1-restart-required.update-notifier\|firefox.cfg\|firefox.sh\|ffox-4-beta-profile-migration-dialog\),,' \
	$(NULL)

include /usr/share/mozilla-devscripts/compare.mk
