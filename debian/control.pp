Source: @APPNAME@
Section: web
Priority: optional
Maintainer: Ubuntu Mozilla Team <ubuntu-mozillateam@lists.ubuntu.com>
Vcs-Bzr: https://code.launchpad.net/~mozillateam/firefox/firefox-4.0.head
Build-Depends: cdbs, 
	debhelper (>= 5), 
	sharutils, 
	m4,
	autotools-dev, 
	autoconf2.13,
	quilt, 
	patchutils (>= 0.2.25),
	bzip2, 
	zip,
	libx11-dev, 
	libxt-dev,
	libgtk2.0-dev (>= 2.10),
	liborbit2-dev, 
	libidl-dev (>= 0.8.0),
	libxft-dev, 
	libfreetype6-dev,
	libxrender-dev, 
	libxinerama-dev,
	libgnome2-dev, 
	libgconf2-dev, 
	libgnomeui-dev,
	libstartup-notification0-dev,
	libasound2-dev,
	libcurl4-openssl-dev,
	libdbus-glib-1-dev (>= 0.60),
	mozilla-devscripts (>= 0.10~),
	hardening-wrapper,
	lsb-release,
	libiw-dev,
	mesa-common-dev,
	libnotify-dev (>= 0.4), @EXTRA_BD@
	libgnomevfs2-dev, 
        yasm,
	xvfb,
Standards-Version: 3.8.1

Package: @APPNAME@
Architecture: any
Depends: @APPNAME@-core (= ${binary:Version}),
	${misc:Depends},
	${shlibs:Depends}
Provides: www-browser, iceweasel
Conflicts: firefox-4.0-branding (<= 4.0~b5~hg20100818r50769+nobinonly-0ubuntu1), 
	@APPNAME_OTHER@
Replaces: firefox-4.0-branding, 
	kubuntu-firefox-installer
Suggests: @APPNAME@-gnome-support (= ${binary:Version}) | firefox-kde-support
XB-Xul-AppId: {ec8030f7-c20a-464f-9b0e-13a3a9e97384}
Description: Safe and easy web browser from Mozilla
 Firefox delivers safe, easy web browsing. A familiar user interface,
 enhanced security features including protection from online identity theft,
 and integrated search let you get the most out of the web.

Package: @APPNAME_OTHER@
Architecture: any
Depends: @APPNAME@-core (= ${binary:Version}), 
	${misc:Depends},
	${shlibs:Depends}
Provides: www-browser, iceweasel
Conflicts: @APPNAME@, 
	abrowser-4.0-branding (<= 4.0~b5~hg20100818r50769+nobinonly-0ubuntu1)
Replaces: abrowser-4.0-branding
Suggests: @APPNAME_OTHER@-gnome-support (= ${binary:Version})
XB-Xul-AppId: {ec8030f7-c20a-464f-9b0e-13a3a9e97384}
Description: Unbranded web browser based on Mozilla
 ABrowser 4.0 is an unbranded version of the popular Firefox webbrowser;
 it is written in the XUL language and designed to be lightweight and
 cross-platform.

Package: @APPNAME@-core
Architecture: any
Depends: fontconfig,
	psmisc,
	lsb-release,
	debianutils (>= 1.16),
	python,
	python-gtk2,
	${misc:Depends},
	${shlibs:Depends}
Recommends: ubufox
Suggests: latex-xft-fonts, libthai0
Replaces: firefox-4.0, 
	firefox-4.0-gnome-support
Conflicts: firefox-4.0-gnome-support (<= 4.0~b3~hg20100804r48791+nobinonly-0ubuntu1), 
	firefox-4.0 (<= 4.0~b8~hg20101111r57316+nobinonly-0ubuntu1~umd1)
Description: Safe and easy web browser from Mozilla
 Firefox delivers safe, easy web browsing. A familiar user interface,
 enhanced security features including protection from online identity theft,
 and integrated search let you get the most out of the web.
 .
 This package contains the common components shared between the
 @APPNAME_OTHER@ and @APPNAME@ packages

Package: @APPNAME@-gnome-support
Architecture: any
Section: gnome
Depends: ${shlibs:Depends}, 
	${misc:Depends}, 
	@APPNAME@ (= ${binary:Version})
Provides: gnome-www-browser
Conflicts: @APPNAME_OTHER@-gnome-support
Description: Support for GNOME in Mozilla Firefox
 This is an extension to Firefox that allows it to use protocol
 handlers from GnomeVFS, such as smb or sftp, and other GNOME
 integration features.

Package: @APPNAME_OTHER@-gnome-support
Architecture: any
Section: gnome
Depends: ${shlibs:Depends}, 
	${misc:Depends}, 
	@APPNAME_OTHER@ (= ${binary:Version})
Provides: gnome-www-browser
Conflicts: @APPNAME@-gnome-support
Description: Support for GNOME in ABrowser
 This is an extension to ABrowser that allows it to use protocol
 handlers from GnomeVFS, such as smb or sftp, and other GNOME
 integration features.

Package: @APPNAME@-dbg
Architecture: any
Section: debug
Priority: extra
Depends: ${shlibs:Depends}, 
	${misc:Depends}, 
	@APPNAME@ (= ${binary:Version}) | @APPNAME_OTHER@ (= ${binary:Version})
Description: @APPNAME@ debug symbols
 Debug symbols for Firefox 4.0.

Package: @APPNAME@-mozsymbols
Architecture: amd64 i386 armel
Section: debug
Priority: extra
Depends: ${shlibs:Depends}, 
	${misc:Depends}
Description: firefox debug symbols for mozilla
 package containing the firefox symbols in a format expected by mozilla's
 breakpad. eventually this package should go away and the symbol upload be
 implemented in soyuz (or other builders that build this package)

# Transitional packages below here

Package: firefox-4.0-branding
Architecture: any
Depends: firefox-4.0, ${misc:Depends}
Description: Transitional package to pull in firefox-4.0

Package: abrowser-4.0-branding
Architecture: all
Depends: abrowser-4.0, ${misc:Depends}
Description: Transitional package to pull in abrowser-4.0

Package: firefox-4.0-gnome-support-dbg
Architecture: any
Section: debug
Priority: extra
Depends: ${misc:Depends}, firefox-4.0-dbg (= ${binary:Version})
Description: Transitional package to pull in firefox-4.0-dbg
