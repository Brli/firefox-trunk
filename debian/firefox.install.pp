debian/tmp/@LIBDIR@/icons @LIBDIR@
debian/tmp/@LIBDIR@/chrome/icons @LIBDIR@/chrome
#ifdef DEB_MIN_SYSDEPS
debian/tmp/@LIBDIR@/omni.jar @LIBDIR@
#else
debian/tmp/@LIBDIR@/chrome/browser-branding*.jar
debian/tmp/@LIBDIR@/chrome/*.manifest
#endif
