Source: @APPNAME@
Section: web
Priority: optional
Maintainer: Ubuntu Mozilla Team <ubuntu-mozillateam@lists.ubuntu.com>
Vcs-Bzr: https://code.launchpad.net/~mozillateam/firefox/firefox-4.0.head
Build-Depends: cdbs, 
	debhelper (>= 5), 
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
	imagemagick
Standards-Version: 3.9.1

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
Recommends: ubufox,
	@APPNAME@-globalmenu
Provides: www-browser,
	iceweasel,
	gnome-www-browser
Suggests: @APPNAME@-gnome-support | firefox-kde-support,
	latex-xft-fonts,
	libthai0
XB-Xul-AppId: {ec8030f7-c20a-464f-9b0e-13a3a9e97384}
Replaces: firefox-4.0-core,
	abrowser-4.0,
	firefox-4.0-gnome-support
Breaks: firefox-4.0-gnome-support (<= 4.0~b3~hg20100804r48791+nobinonly-0ubuntu1), 
	firefox-4.0-core (<= 4.0~b12~hg20110209r62212+nobinonly-0ubuntu1),
	abrowser-4.0 (<= 4.0~b12~hg20110209r62212+nobinonly-0ubuntu1)
Description: Safe and easy web browser from Mozilla
 Firefox delivers safe, easy web browsing. A familiar user interface,
 enhanced security features including protection from online identity theft,
 and integrated search let you get the most out of the web.

Package: @APPNAME@-gnome-support
Architecture: any
Section: gnome
Depends: ${shlibs:Depends}, 
	${misc:Depends}, 
	@APPNAME@
Description: Safe and easy web browser from Mozilla - GNOME support
 Firefox delivers safe, easy web browsing. A familiar user interface,
 enhanced security features including protection from online identity theft,
 and integrated search let you get the most out of the web.
 .
 This package depends on the GNOME libraries which allow Firefox to take
 advantage of technologies such as GConf, GIO libnotify

Package: @APPNAME@-dbg
Architecture: any
Section: debug
Priority: extra
Depends: ${shlibs:Depends}, 
	${misc:Depends}, 
	@APPNAME@ (= ${binary:Version})
Description: Safe and easy web browser from Mozilla - debug symbols
 Firefox delivers safe, easy web browsing. A familiar user interface,
 enhanced security features including protection from online identity theft,
 and integrated search let you get the most out of the web.
 .
 This package contains the debugging symbols for the Firefox web
 browser

Package: @APPNAME@-dev
Architecture: any
Section: devel
Priority: extra
Depends: ${shlibs:Depends},
	${misc:Depends},
	@APPNAME@ (= ${binary:Version}),
	${nspr:Depends},
	${nss:Depends},
	${cairo:Depends}
Description: Safe and easy web browser from Mozilla - development files
 Firefox delivers safe, easy web browsing. A familiar user interface,
 enhanced security features including protection from online identity theft,
 and integrated search let you get the most out of the web.
 .
 This package contains the headers and SDK for building plugins with Firefox

Package: @APPNAME@-mozsymbols
Architecture: amd64 i386 armel
Section: debug
Priority: extra
Depends: ${shlibs:Depends}, 
	${misc:Depends}
Description: Safe and easy web browser from Mozilla - Breakpad symbols
 Firefox delivers safe, easy web browsing. A familiar user interface,
 enhanced security features including protection from online identity theft,
 and integrated search let you get the most out of the web.
 .
 This package contains the Firefox symbols in a format expected by Mozilla's
 Breakpad. Eventually this package should go away and the symbol upload be
 implemented in soyuz (or other builders that build this package)

Package: @APPNAME@-globalmenu
Architecture: any
Depends: ${shlibs:Depends},
	${misc:Depends},
	@APPNAME@ (= ${binary:Version})
Description: Unity appmenu integration for Firefox
 This package provides an extension which adds support for the Unity
 appmenu to Firefox 

# Transitional packages below here

Package: firefox-4.0-branding
Architecture: any
Depends: firefox-4.0, ${misc:Depends}
Description: Safe and easy web browser from Mozilla - transitional package
 This is a transitional package to ensure that upgrades work correctly.
 It can be safely removed

Package: abrowser-4.0-branding
Architecture: all
Depends: firefox-4.0, ${misc:Depends}
Description: Safe and easy web browser from Mozilla - transitional package
 This is a transitional package to ensure that upgrades work correctly.
 It can be safely removed

Package: firefox-4.0-gnome-support-dbg
Architecture: any
Section: debug
Priority: extra
Depends: ${misc:Depends}, firefox-4.0-gnome-support, firefox-4.0-dbg (= ${binary:Version})
Description: Safe and easy web browser from Mozilla - transitional package
 This is a transitional package to ensure that upgrades work correctly.
 It can be safely removed

Package: abrowser-4.0
Architecture: any
Depends: ${misc:Depends}, firefox-4.0
Description: Safe and easy web browser from Mozilla - transitional package
 This is a transitional package to ensure that upgrades work correctly.
 It can be safely removed

Package: firefox-4.0-core
Architecture: any
Depends: ${misc:Depends}, firefox-4.0
Description: Safe and easy web browser from Mozilla - transitional package
 This is a transitional package to ensure that upgrades work correctly.
 It can be safely removed

Package: abrowser-4.0-gnome-support
Architecture: any
Depends: ${misc:Depends}, firefox-4.0-gnome-support
Description: Safe and easy web browser from Mozilla - transitional package
 This is a transitional package to ensure that upgrades work correctly.
 It can be safely removed
