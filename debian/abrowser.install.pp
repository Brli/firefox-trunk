debian/tmp-@APPNAME_OTHER@-branding/@LIBDIR@/icons @LIBDIR@
debian/tmp-@APPNAME_OTHER@-branding/@LIBDIR@/chrome/icons @LIBDIR@/chrome
#ifdef DEB_MIN_SYSDEPS
debian/tmp-@APPNAME_OTHER@-branding/@LIBDIR@/omni.jar @LIBDIR@
#else
debian/tmp-@APPNAME_OTHER@-branding/@LIBDIR@/chrome/browser-branding*.jar
debian/tmp-@APPNAME_OTHER@-branding/@LIBDIR@/chrome/*.manifest
#endif