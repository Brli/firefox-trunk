ac_add_options --build=@DEB_BUILD_GNU_TYPE@
#ifdef DEB_HOST_GNU_TYPE
ac_add_options --host=@DEB_HOST_GNU_TYPE@
#endif
ac_add_options --prefix=@DEB_CONFIGURE_PREFIX@
ac_add_options --localstatedir=@DEB_CONFIGURE_LOCALSTATEDIR@
ac_add_options --libexecdir=@DEB_CONFIGURE_LIBEXECDIR@
ac_add_options --disable-maintainer-mode
ac_add_options --disable-dependency-tracking
ac_add_options --disable-silent-rules
ac_add_options --srcdir=@TOPSRCDIR@/@DEB_BUILDDIR@
mk_add_options MOZ_OBJDIR=@TOPSRCDIR@/@MOZ_OBJDIR@
ac_add_options --disable-elf-dynstr-gc
ac_add_options --disable-install-strip
ac_add_options --disable-strip
ac_add_options --disable-updater
ac_add_options --enable-application=browser
ac_add_options --enable-default-toolkit=cairo-gtk2
ac_add_options --enable-gnomevfs
ac_add_options --enable-pango
ac_add_options --enable-svg
ac_add_options --enable-mathml
ac_add_options --enable-safe-browsing
ac_add_options --with-distribution-id=com.ubuntu
ac_add_options --enable-gio
#ifndef DEB_MIN_SYSDEPS
ac_add_options --with-system-jpeg=/usr
ac_add_options --with-system-png=/usr
ac_add_options --with-system-zlib=/usr
ac_add_options --enable-system-hunspell
ac_add_options --with-libxul-sdk=@DEBIAN_XUL_DEV@
ac_add_options --with-system-libxul
ac_add_options --enable-chrome-format=jar
#else
ac_add_options --without-system-jpeg
ac_add_options --without-system-png
ac_add_options --without-system-zlib
ac_add_options --disable-system-hunspell
ac_add_options --enable-chrome-format=omni
#endif
#ifndef DEB_MOZ_VALGRIND
#ifdef DEB_NO_OPTIMIZE
ac_add_options --disable-optimize
#else
ac_add_options --enable-optimize
#endif
#else
ac_add_options --enable-optimize="-g -O -freorder-blocks"
ac_add_options --disable-jemalloc
ac_add_options --enable-valgrind
mk_add_options MOZ_MAKE_FLAGS=-j4
#endif
#ifdef DEB_ENABLE_IPC
ac_add_options --enable-ipc
#else
ac_add_options --disable-ipc
#endif
#ifdef DEB_WANT_UNIT_TESTS
ac_add_options --enable-tests
ac_add_options --enable-mochitest
#ifdef DEB_ENABLE_IPC
ac_add_options --enable-ipdl-tests
#else
ac_add_options --disable-ipdl-tests
#endif
#else
ac_add_options --disable-tests
ac_add_options --disable-mochitest
ac_add_options --disable-ipdl-tests
#endif
#ifdef USE_SYSTEM_CAIRO
ac_add_options --enable-system-cairo
#else
ac_add_options --disable-system-cairo
#endif
#ifdef USE_SYSTEM_NSPR
ac_add_options --with-system-nspr
#else
ac_add_options --without-system-nspr
#endif
#ifdef USE_SYSTEM_NSS
ac_add_options --with-system-nss
#else
ac_add_options --without-system-nss
#endif
#ifdef USE_SYSTEM_SQLITE
ac_add_options --enable-system-sqlite
#else
ac_add_options --disable-system-sqlite
#endif
#ifdef DEB_ENABLE_BREAKPAD
ac_add_options --enable-crashreporter
#else
ac_add_options --disable-crashreporter
#endif
ac_add_options @BRANDING@
#ifdef DEB_BUILD_PGO
mk_add_options PROFILE_GEN_SCRIPT='xvfb-run -a @PYTHON@ @TOPSRCDIR@/@MOZ_OBJDIR@/_profile/pgo/profileserver.py'
#endif
#ifdef DISABLE_GNOMEVFS
ac_add_options --disable-gnomevfs
#endif
