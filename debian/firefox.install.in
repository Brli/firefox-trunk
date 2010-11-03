debian/tmp/@LIBDIR@/components/*.manifest
debian/tmp/@LIBDIR@/components/*.so
debian/tmp/@LIBDIR@/icons @LIBDIR@
debian/tmp/@LIBDIR@/chrome/icons @LIBDIR@/chrome

#ifndef DEB_MIN_SYSDEPS
debian/tmp/@LIBDIR@/components/*.js
debian/tmp/@LIBDIR@/components/*.xpt
debian/tmp/@LIBDIR@/chrome/en-US.jar
debian/tmp/@LIBDIR@/chrome/browser.jar
debian/tmp/@LIBDIR@/chrome/*.manifest
debian/tmp/@LIBDIR@/defaults/autoconfig
debian/tmp/@LIBDIR@/defaults/preferences/[a-u]*.js
debian/tmp/@LIBDIR@/defaults/profile
debian/tmp/@LIBDIR@/modules
#else
debian/tmp/@LIBDIR@/*.so
debian/tmp/@LIBDIR@/@APPNAME@-bin
debian/tmp/@LIBDIR@/omni.jar @LIBDIR@
#ifdef DEB_ENABLE_BREAKPAD
debian/tmp/@LIBDIR@/crashreporter
#endif
#ifdef DEB_ENABLE_IPC
debian/tmp/@LIBDIR@/plugin-container
#endif
#endif

#ifdef DEB_ENABLE_BREAKPAD
debian/apport/@APPNAME@ etc/apport/blacklist.d
#endif

debian/tmp/@LIBDIR@/run-mozilla.sh
debian/tmp/@LIBDIR@/@APPNAME@
debian/tmp/@LIBDIR@/chrome.manifest
debian/tmp/@LIBDIR@/searchplugins/* usr/lib/@APPNAME@-addons/searchplugins/en-US
debian/tmp/@LIBDIR@/blocklist.xml

debian/@APPNAME@.desktop usr/share/applications

debian/presubj usr/share/bug/@APPNAME@
debian/firefox.sh @LIBDIR@
debian/@APPNAME@-restart-required.update-notifier @LIBDIR@
debian/migrator/ffox-32-beta-profile-migration-dialog @LIBDIR@
debian/distribution.ini @LIBDIR@/distribution
debian/usr.bin.@APPNAME@ etc/apparmor.d

#debian/debsearch.gif usr/lib/@APPNAME@-addons/searchplugins
#debian/debsearch.src usr/lib/@APPNAME@-addons/searchplugins

debian/tmp/@LIBDIR@/extensions/\{972ce4c6-7e08-4474-a285-3208198ce6fd\} usr/lib/@APPNAME@-addons/extensions

# debian/tmp/@LIBDIR@/plugins/*.so usr/lib/@APPNAME@-addons/plugins

debian/tmp/@LIBDIR@/*.ini

# Don't install the apport hook for now.
# debian/apport/@APPNAME@.py usr/share/apport/package-hooks/
