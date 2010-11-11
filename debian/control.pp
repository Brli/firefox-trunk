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
Depends: fontconfig,
	psmisc,
	lsb-release,
	debianutils (>= 1.16),
	python,
	python-gtk2,
	${misc:Depends},
	${shlibs:Depends}
Recommends: ubufox
Provides: www-browser, iceweasel
Conflicts: firefox-4.0-branding (<= 4.0~b5~hg20100818r50769+nobinonly-0ubuntu1), 
        firefox-4.0-gnome-support (<= 4.0~b3~hg20100804r48791+nobinonly-0ubuntu1),
	firefox-4.0-core (<= 4.0~b8~hg20101103r56843+nobinonly-0ubuntu1)
Replaces: firefox-4.0-branding, 
	kubuntu-firefox-installer,
        firefox-4.0-gnome-support,
	firefox-4.0-core
Suggests: @APPNAME@-gnome-support (= ${binary:Version}),
	latex-xft-fonts,
	libthai0
XB-Xul-AppId: {ec8030f7-c20a-464f-9b0e-13a3a9e97384}
Description: safe and easy web browser from Mozilla
 Firefox delivers safe, easy web browsing. A familiar user interface,
 enhanced security features including protection from online identity theft,
 and integrated search let you get the most out of the web.

Package: @APPNAME@-gnome-support
Architecture: any
Section: gnome
Depends: ${shlibs:Depends}, 
	${misc:Depends}, 
	@APPNAME@ (= ${binary:Version})
Provides: gnome-www-browser
Description: Support for GNOME in Mozilla Firefox
 This is an extension to Firefox that allows it to use protocol
 handlers from GnomeVFS, such as smb or sftp, and other GNOME
 integration features.

Package: @APPNAME@-dbg
Architecture: any
Section: debug
Priority: extra
Depends: ${shlibs:Depends}, 
	${misc:Depends}, 
	@APPNAME@ (= ${binary:Version})
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

Package: firefox-4.0-core
Architecture: any
Depends: firefox-4.0 (= ${binary:Version}), ${misc:Depends}
Description: Transitional package to pull in firefox-4.0

Package: firefox-4.0-gnome-support-dbg
Architecture: any
Section: debug
Priority: extra
Depends: ${misc:Depends}, firefox-4.0-dbg (= ${binary:Version})
Description: Transitional package to pull in firefox-4.0-dbg
