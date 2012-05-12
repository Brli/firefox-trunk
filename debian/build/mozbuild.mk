#!/usr/bin/make -f

# Various build defaults
# 1 = Enable crashreporter (if supported)
MOZ_ENABLE_BREAKPAD	?= 0
# 1 = Disable official branding and crash reporter (the crash reporter builds but is not enabled in application.ini)
MOZ_BUILD_UNOFFICIAL	?= 1
# 1 = Build without jemalloc suitable for valgrind debugging
MOZ_VALGRIND		?= 0
# 1 = Profile guided build
MOZ_BUILD_PGO		?= 0
# 1 = Build and run the testsuite
MOZ_WANT_UNIT_TESTS	?= 0
# 1 = Turn on debugging bits and disable optimizations
MOZ_DEBUG		?= 0
# 1 = Disable optimizations
MOZ_NO_OPTIMIZE		?= 0
# The binary name to use (derived from the package name by default, but can be overridden)
MOZ_APP_NAME		?= $(MOZ_PKG_NAME)
# The value of "Name" to use in application.ini, which the profile location is based on.
# Derived from the desired MOZ_APP_NAME, but can be overridden
MOZ_APP_BASENAME	?= $(shell echo $(MOZ_APP_NAME) | sed -n 's/\-.\|\<./\U&/g p')
# The default value of "Name" in the application.ini, derived from the upstream build system
# It is used for the profile location. This should be set manually if not provided
# XXX: Needs the tarball unpacked, but we need this before the tarball is unpacked
#MOZ_DEFAULT_APP_BASENAME ?= $(shell . $(DEB_BUILDDIR)/$(MOZ_APP)/confvars.sh; echo $$MOZ_APP_BASENAME)
# Equal to upstreams default MOZ_APP_NAME. If not a lower case version of the "Name"
# in application.ini, then this should be manually overridden
MOZ_DEFAULT_APP_NAME	?= $(MOZ_DEFAULT_APP_BASENAME_L)
# Location for searchplugins
MOZ_SEARCHPLUGIN_DIR	?= $(MOZ_LIBDIR)/distribution/searchplugins

# These are used for cross-compiling and for saving the configure script
# from having to guess our platform (since we know it already)
DEB_HOST_GNU_TYPE	:= $(shell dpkg-architecture -qDEB_HOST_GNU_TYPE)
DEB_BUILD_GNU_TYPE	:= $(shell dpkg-architecture -qDEB_BUILD_GNU_TYPE)
DEB_HOST_ARCH		:= $(shell dpkg-architecture -qDEB_HOST_ARCH)
DEB_HOST_GNU_CPU	:= $(shell dpkg-architecture -qDEB_HOST_GNU_CPU)
DEB_HOST_GNU_SYSTEM	:= $(shell dpkg-architecture -qDEB_HOST_GNU_SYSTEM)
# Other things which should be defined before including the CDBS rules
DEB_TAR_SRCDIR		:= mozilla

DISTRIB_VERSION_MAJOR 	:= $(shell lsb_release -s -r | cut -d '.' -f 1)
DISTRIB_VERSION_MINOR 	:= $(shell lsb_release -s -r | cut -d '.' -f 2)
DISTRIB_CODENAME	:= $(shell lsb_release -s -c)

# We need this to execute before the debian/control target gets called
clean:: pre-auto-update-debian-control

# We need this to run before apply-patches
post-patches:: enable-dist-patches

-include /usr/share/cdbs/1/rules/tarball.mk
-include /usr/share/cdbs/1/rules/debhelper.mk
-include /usr/share/cdbs/1/rules/patchsys-quilt.mk
-include /usr/share/cdbs/1/class/makefile.mk
include $(CURDIR)/debian/build/get-orig-source.mk

MOZ_OBJDIR		:= $(DEB_BUILDDIR)/obj-$(DEB_HOST_GNU_TYPE)
MOZ_DISTDIR		:= $(MOZ_OBJDIR)/$(MOZ_MOZDIR)/dist

# Define other variables used throughout the build
# The package name
MOZ_PKG_NAME		:= $(shell dpkg-parsechangelog | sed -n 's/^Source: *\(.*\)$$/\1/ p')

MOZ_APP_BASENAME_L	= $(shell echo $(MOZ_APP_BASENAME) | tr A-Z a-z)
MOZ_DEFAULT_APP_BASENAME_L = $(shell echo $(MOZ_DEFAULT_APP_BASENAME) | tr A-Z a-z)

DEB_MAKE_MAKEFILE	:= client.mk
# Without this, CDBS passes CFLAGS and CXXFLAGS options to client.mk, which breaks the build
DEB_MAKE_EXTRA_ARGS	:=
# These normally come from autotools.mk, which we no longer include (because we
# don't want to run configure)
DEB_MAKE_INSTALL_TARGET	:= install DESTDIR=$(CURDIR)/debian/tmp
DEB_MAKE_CLEAN_TARGET	:= distclean
DEB_DH_STRIP_ARGS	:= --dbg-package=$(MOZ_PKG_NAME)-dbg
# We don't want build-tree/mozilla/README to be shipped as a doc
DEB_INSTALL_DOCS_ALL 	:= $(NULL)

MOZ_VERSION		= $(shell cat $(DEB_BUILDDIR)/$(MOZ_APP)/config/version.txt)
MOZ_LIBDIR		:= usr/lib/$(MOZ_APP_NAME)
MOZ_INCDIR		:= usr/include/$(MOZ_APP_NAME)
MOZ_IDLDIR		:= usr/share/idl/$(MOZ_APP_NAME)
MOZ_SDKDIR		:= usr/lib/$(MOZ_APP_NAME)-devel
MOZ_ADDONDIR		:= usr/lib/$(MOZ_APP_NAME)-addons

# The profile directory is determined from the Vendor and Name fields of
# the application.ini
ifeq (,$(MOZ_VENDOR))
PROFILE_BASE =
else
PROFILE_BASE = $(shell echo $(MOZ_VENDOR) | tr A-Z a-z)/
endif
MOZ_PROFILEDIR		= .$(PROFILE_BASE)$(MOZ_APP_BASENAME_L)
MOZ_DEFAULT_PROFILEDIR	= .$(PROFILE_BASE)$(MOZ_DEFAULT_APP_BASENAME_L)

DEB_AUTO_UPDATE_DEBIAN_CONTROL	= no

MOZ_PYTHON		:= $(shell which python)
DISTRIB 		:= $(shell lsb_release -i -s)

NO_AUTO_REFRESH_LOCALES	?= 0

CFLAGS			= -g
CXXFLAGS		= -g
LDFLAGS 		= $(shell echo $$LDFLAGS | sed -e 's/-Wl,-Bsymbolic-functions//')

ifneq (,$(findstring nocheck,$(DEB_BUILD_OPTIONS)))
MOZ_WANT_UNIT_TESTS = 0
endif

ifeq (1,$(MOZ_VALGRIND))
MOZ_BUILD_UNOFFICIAL = 1
endif

ifneq (,$(findstring noopt,$(DEB_BUILD_OPTIONS)))
MOZ_BUILD_PGO = 0
MOZ_NO_OPTIMIZE	= 1
endif

ifneq (,$(findstring debug,$(DEB_BUILD_OPTIONS)))
MOZ_NO_OPTIMIZE = 1
MOZ_DEBUG = 1
MOZ_BUILD_UNOFFICIAL = 1
endif

include $(CURDIR)/debian/build/testsuite.mk

ifneq ($(MOZ_APP_NAME)$(MOZ_APP_BASENAME),$(MOZ_DEFAULT_APP_NAME)$(MOZ_DEFAULT_APP_BASENAME))
# If we change MOZ_APP_NAME or MOZ_APP_BASENAME, don't use official branding
MOZ_BUILD_UNOFFICIAL = 1
endif

# enable the crash reporter only on i386, amd64 and armel
ifeq (,$(filter i386 amd64 armel,$(DEB_HOST_ARCH)))
MOZ_ENABLE_BREAKPAD = 0
endif

# powerpc sucks
ifneq (,$(filter powerpc,$(DEB_HOST_ARCH)))
MOZ_WANT_UNIT_TESTS = 0
endif

# Ensure the crash reporter gets disabled for derivatives
ifneq (Ubuntu, $(DISTRIB))
MOZ_ENABLE_BREAKPAD = 0
endif

ifeq (,$(filter lucid maverick natty oneiric precise, $(DISTRIB_CODENAME)))
MOZ_ENABLE_BREAKPAD = 0
endif

MOZ_DISPLAY_NAME = $(shell cat $(DEB_BUILDDIR)/$(MOZ_BRANDING_DIR)/locales/en-US/brand.properties \
		    | grep brandShortName | sed -e 's/brandShortName\=//')

ifneq (,$(filter lucid maverick natty, $(DISTRIB_CODENAME)))
MOZ_BUILD_PGO = 0
endif

ifeq (,$(filter lucid maverick, $(DISTRIB_CODENAME)))
MOZ_ENABLE_GLOBALMENU = 1
endif

ifeq (,$(filter lucid, $(DISTRIB_CODENAME)))
MOZ_SYSTEM_DICTDIR = /usr/share/hunspell
else
MOZ_SYSTEM_DICTDIR = /usr/share/myspell/dicts
endif

export LDFLAGS
export DEB_BUILD_HARDENING=1
export MOZCONFIG=$(CURDIR)/debian/config/mozconfig
ifeq (Ubuntu, $(DISTRIB))
export MOZ_UA_VENDOR=Ubuntu
endif
ifneq (1,$(MOZ_BUILD_UNOFFICIAL))
export BUILD_OFFICIAL=1
endif
ifeq (1,$(MOZ_ENABLE_BREAKPAD))
# Needed to enable crashreported in application.ini
export MOZILLA_OFFICIAL=1
endif

ifeq (linux-gnu, $(DEB_HOST_GNU_SYSTEM))
LANGPACK_DIR := linux-$(DEB_HOST_GNU_CPU)/xpi
else
LANGPACK_DIR := $(DEB_HOST_GNU_SYSTEM)-$(DEB_HOST_GNU_CPU)/xpi
endif

ifneq ($(MOZ_PKG_NAME),$(MOZ_APP_NAME))
$(foreach pkg, APP GM GS DEV DBG MS, $(foreach rel, CONFLICTS PROVIDES, $(ifneq ,$(PKG_$(pkg)_$(rel)_ARGS), eval $(PKG_$(pkg)_$(rel)_ARGS) += ", ")))
PKG_APP_CONFLICTS_ARGS += "$(MOZ_APP_NAME)"
PKG_APP_PROVIDES_ARGS += "$(MOZ_APP_NAME)"
PKG_GM_CONFLICTS_ARGS += "$(MOZ_APP_NAME)-globalmenu"
PKG_GM_PROVIDES_ARGS += "$(MOZ_APP_NAME)-globalmenu"
PKG_GS_CONFLICTS_ARGS += "$(MOZ_APP_NAME)-gnome-support"
PKG_GS_PROVIDES_ARGS += "$(MOZ_APP_NAME)-gnome-support"
PKG_DEV_CONFLICTS_ARGS += "$(MOZ_APP_NAME)-dev"
PKG_DEV_PROVIDES_ARGS += "$(MOZ_APP_NAME)-dev"
PKG_DBG_CONFLICTS_ARGS += "$(MOZ_APP_NAME)-dbg"
PKG_DBG_PROVIDES_ARGS += "$(MOZ_APP_NAME)-dbg"
PKG_MS_CONFLICTS_ARGS += "$(MOZ_APP_NAME)-mozsymbols"
PKG_MS_PROVIDES_ARGS += "$(MOZ_APP_NAME)-mozsymbols"
endif

ifneq (,$(filter lucid maverick natty, $(DISTRIB_CODENAME)))
GCONF_DEPENDS := libgconf2-4
endif

DEB_DH_GENCONTROL_ARGS_$(MOZ_PKG_NAME) := -- -Vapp:Replaces="$(PKG_APP_REPLACES_ARGS)" -Vapp:Breaks="$(PKG_APP_BREAKS_ARGS)" -Vapp:Conflicts="$(PKG_APP_CONFLICTS_ARGS)" \
					     -Vapp:Provides="$(PKG_APP_PROVIDES_ARGS)" $(PKG_APP_EXTRA_ARGS) -Vsupport:Suggests="$(PKG_SUPPORT_SUGGESTS)" \
					     -Vsupport:Recommends="$(PKG_SUPPORT_RECOMMENEDS)"
DEB_DH_GENCONTROL_ARGS_$(MOZ_PKG_NAME)-globalmenu := -- -Vgm:Replaces="$(PKG_GM_REPLACES_ARGS)" -Vgm:Breaks="$(PKG_GM_BREAKS_ARGS)" -Vgm:Conflicts="$(PKG_GM_CONFLICTS_ARGS)" \
							-Vgm:Provides="$(PKG_GM_PROVIDES_ARGS)" $(PKG_GM_EXTRA_ARGS)
DEB_DH_GENCONTROL_ARGS_$(MOZ_PKG_NAME)-gnome-support := -- -Vgs:Replaces="$(PKG_GS_REPLACES_ARGS)" -Vgs:Breaks="$(PKG_GS_BREAKS_ARGS)" -Vgs:Conflicts="$(PKG_GS_CONFLICTS_ARGS)" \
							   -Vgs:Provides="$(PKG_GS_PROVIDES_ARGS)" -Vgconf:Depends="$(GCONF_DEPENDS)" $(PKG_GS_EXTRA_ARGS)
DEB_DH_GENCONTROL_ARGS_$(MOZ_PKG_NAME)-dev := -- -Vdev:Replaces="$(PKG_DEV_REPLACES_ARGS)" -Vdev:Breaks="$(PKG_DEV_BREAKS_ARGS)" -Vdev:Conflicts="$(PKG_DEV_CONFLICTS_ARGS)" \
						 -Vdev:Provides="$(PKG_DEV_PROVIDES_ARGS)" $(PKG_DEV_EXTRA_ARGS)
DEB_DH_GENCONTROL_ARGS_$(MOZ_PKG_NAME)-dbg := -- -Vdbg:Replaces="$(PKG_DBG_REPLACES_ARGS)" -Vdbg:Breaks="$(PKG_DBG_BREAKS_ARGS)" -Vdbg:Conflicts="$(PKG_DBG_CONFLICTS_ARGS)" \
						 -Vdbg:Provides="$(PKG_DBG_PROVIDES_ARGS)" $(PKG_DBG_EXTRA_ARGS)
DEB_DH_GENCONTROL_ARGS_$(MOZ_PKG_NAME)-mozsymbols := -- -Vms:Replaces="$(PKG_MS_REPLACES_ARGS)" -Vms:Breaks="$(PKG_MS_BREAKS_ARGS)" -Vms:Conflicts="$(PKG_MS_CONFLICTS_ARGS)" \
							-Vms:Provides="$(PKG_MS_PROVIDES_ARGS)" $(PKG_MS_EXTRA_ARGS)

# Defines used for the Mozilla text preprocessor
DEB_DEFINES += 	-DMOZ_LIBDIR="$(MOZ_LIBDIR)" -DMOZ_APP_NAME="$(MOZ_APP_NAME)" -DMOZ_APP_BASENAME="$(MOZ_APP_BASENAME)" \
		-DMOZ_INCDIR="$(MOZ_INCDIR)" -DMOZ_IDLDIR="$(MOZ_IDLDIR)" -DMOZ_VERSION="$(MOZ_VERSION)" -DDEB_HOST_ARCH="$(DEB_HOST_ARCH)" \
		-DMOZ_DISPLAY_NAME="$(MOZ_DISPLAY_NAME)" -DMOZ_SYSTEM_DICTDIR="$(MOZ_SYSTEM_DICTDIR)" -DMOZ_PKG_NAME="$(MOZ_PKG_NAME)" \
		-DMOZ_BRANDING_OPTION="$(MOZ_BRANDING_OPTION)" -DTOPSRCDIR="$(CURDIR)" -DDEB_HOST_GNU_TYPE="$(DEB_HOST_GNU_TYPE)" \
		-DMOZ_ADDONDIR="$(MOZ_ADDONDIR)" -DMOZ_SDKDIR="$(MOZ_SDKDIR)" -DMOZ_DISTDIR="$(MOZ_DISTDIR)" -DMOZ_UPDATE_CHANNEL="$(CHANNEL)" \
		-DMOZ_OBJDIR="$(MOZ_OBJDIR)" -DDEB_BUILDDIR="$(DEB_BUILDDIR)" -DMOZ_PYTHON="$(MOZ_PYTHON)" -DMOZ_PROFILEDIR="$(MOZ_PROFILEDIR)" \
		-DMOZ_PKG_BASENAME="$(MOZ_PKG_BASENAME)" -DMOZ_DEFAULT_PROFILEDIR="$(MOZ_DEFAULT_PROFILEDIR)" \
		-DMOZ_DEFAULT_APP_NAME="$(MOZ_DEFAULT_APP_NAME)" -DMOZ_DEFAULT_APP_BASENAME="$(MOZ_DEFAULT_APP_BASENAME)"

ifeq (1, $(MOZ_ENABLE_BREAKPAD))
DEB_DEFINES += -DMOZ_ENABLE_BREAKPAD
endif
ifeq (1, $(MOZ_VALGRIND))
DEB_DEFINES += -DMOZ_VALGRIND
endif
ifeq (1,$(MOZ_NO_OPTIMIZE))
DEB_DEFINES += -DMOZ_NO_OPTIMIZE
endif
ifeq (1,$(MOZ_WANT_UNIT_TESTS))
DEB_DEFINES += -DMOZ_WANT_UNIT_TESTS
endif
ifneq ($(DEB_BUILD_GNU_TYPE),$(DEB_HOST_GNU_TYPE))
DEB_DEFINES += -DDEB_BUILD_GNU_TYPE="$(DEB_BUILD_GNU_TYPE)"
endif
ifeq (1,$(MOZ_BUILD_PGO))
DEB_DEFINES += -DMOZ_BUILD_PGO
endif
ifeq (1,$(MOZ_DEBUG))
DEB_DEFINES += -DMOZ_DEBUG
endif
ifeq (1,$(MOZ_ENABLE_GLOBALMENU))
DEB_DEFINES += -DMOZ_ENABLE_GLOBALMENU
endif
ifeq (official, $(BRANDING))
DEB_DEFINES += -DMOZ_OFFICIAL_BRANDING
endif
ifeq (,$(filter $(MOZ_OLD_SYSPREF_RELEASES), $(DISTRIB_CODENAME)))
DEB_DEFINES += -DMOZ_NEW_SYSPREF
endif
ifeq (,$(filter lucid maverick natty oneiric, $(DISTRIB_CODENAME)))
DEB_DEFINES += -DMOZ_FREEDESKTOP_ACTIONS
endif
ifneq (,$(filter lucid maverick natty oneiric precise, $(DISTRIB_CODENAME)))
ifneq (,$(filter armel,$(DEB_HOST_ARCH)))
DEB_DEFINES += -DMOZ_ENABLE_THUMB
endif
endif

DEBIAN_EXECUTABLES = $(MOZ_PKG_NAME)/$(MOZ_LIBDIR)/$(MOZ_PKG_BASENAME).sh \
		     $(NULL)

pkgname_subst_files = \
	debian/$(MOZ_PKG_BASENAME).sh \
	debian/apport/blacklist \
	debian/apport/native-origins \
	debian/apport/source_$(MOZ_PKG_NAME).py \
	debian/config/mozconfig \
	$(MOZ_PKGNAME_SUBST_FILES) \
	$(NULL)

$(foreach pkg, $(MOZ_PKG_NAME) $(MOZ_PKG_NAME)-gnome-support $(MOZ_PKG_NAME)-globalmenu $(MOZ_PKG_NAME)-dev $(MOZ_PKG_NAME)-dbg $(MOZ_PKG_NAME)-mozsymbols, \
	$(foreach dhfile, install dirs links manpages postinst preinst postrm prerm lintian-overrides, $(eval pkgname_subst_files += debian/$(pkg).$(dhfile))))

appname_subst_files = \
	debian/$(MOZ_APP_NAME).desktop \
	debian/$(MOZ_APP_NAME).1 \
	$(MOZ_APPNAME_SUBST_FILES) \
	$(NULL)

pkgconfig_files = \
	$(MOZ_PKGCONFIG_FILES) \
	$(NULL)

debian/control:: debian/control.in debian/control.langpacks debian/control.langpacks.unavail debian/config/locales.shipped debian/config/locales.all
	@echo ""
	@echo "*****************************"
	@echo "* Refreshing debian/control *"
	@echo "*****************************"
	@echo ""

	@sed -e 's/@MOZ_PKG_NAME@/$(MOZ_PKG_NAME)/g' < debian/control.in > debian/control
	@perl debian/build/dump-langpack-control-entries.pl -i $(CURDIR)/debian/config -t $(CURDIR)/debian > debian/control.tmp
	@sed -e 's/@MOZ_PKG_NAME@/$(MOZ_PKG_NAME)/g' < debian/control.tmp >> debian/control && rm -f debian/control.tmp

$(foreach file, $(pkgname_subst_files), $(eval pkgname_subst_files_deps += \
	$(shell if [ -f $(CURDIR)/$(subst $(MOZ_PKG_NAME),$(MOZ_PKG_BASENAME),$(file).in) ]; then echo $(subst $(MOZ_PKG_NAME),$(MOZ_PKG_BASENAME),$(file).in); fi)))
$(pkgname_subst_files): $(pkgname_subst_files_deps)
	@if [ -e $(CURDIR)/$(subst $(MOZ_PKG_NAME),$(MOZ_PKG_BASENAME),$@.in) ] ; then \
		$(MOZ_PYTHON) $(CURDIR)/$(DEB_BUILDDIR)/$(MOZ_MOZDIR)/config/Preprocessor.py -Fsubstitution --marker="%%" \
			$(DEB_DEFINES) $(CURDIR)/$(subst $(MOZ_PKG_NAME),$(MOZ_PKG_BASENAME),$@.in) > $(CURDIR)/$@ ; \
	fi

$(foreach file, $(appname_subst_files), $(eval appname_subst_files_deps += \
	$(shell if [ -f $(CURDIR)/$(subst $(MOZ_APP_NAME),$(MOZ_PKG_BASENAME),$(file).in) ]; then echo $(subst $(MOZ_APP_NAME),$(MOZ_PKG_BASENAME),$(file).in); fi)))
$(appname_subst_files): $(appname_subst_files_deps)
	@if [ -e $(CURDIR)/$(subst $(MOZ_APP_NAME),$(MOZ_PKG_BASENAME),$@.in) ] ; then \
		$(MOZ_PYTHON) $(CURDIR)/$(DEB_BUILDDIR)/$(MOZ_MOZDIR)/config/Preprocessor.py -Fsubstitution --marker="%%" \
			$(DEB_DEFINES) $(CURDIR)/$(subst $(MOZ_APP_NAME),$(MOZ_PKG_BASENAME),$@.in) > $(CURDIR)/$@ ; \
	fi

%.pc: WCHAR_CFLAGS = $(shell cat $(MOZ_OBJDIR)/config/autoconf.mk | grep WCHAR_CFLAGS | sed 's/^[^=]*=[[:space:]]*\(.*\)$$/\1/')
%.pc: %.pc.in debian/stamp-makefile-build
	$(MOZ_PYTHON) $(DEB_BUILDDIR)/$(MOZ_MOZDIR)/config/Preprocessor.py -Fsubstitution --marker="%%" $(DEB_DEFINES) -DWCHAR_CFLAGS="$(WCHAR_CFLAGS)" $(CURDIR)/$< > $(CURDIR)/$@

make-buildsymbols: debian/stamp-makebuildsymbols
debian/stamp-makebuildsymbols: debian/stamp-makefile-build
ifeq (1, $(MOZ_ENABLE_BREAKPAD))
	# create build symbols
	cd $(MOZ_OBJDIR); \
	        $(MAKE) buildsymbols MOZ_SYMBOLS_EXTRA_BUILDID=$(shell date -d "`dpkg-parsechangelog | grep Date: | sed -e 's/^Date: //'`" +%y%m%d%H%M%S)-$(DEB_HOST_GNU_CPU)
endif
	touch $@

compare-locales/scripts/compare-locales:
	cp -r $(CURDIR)/debian/build/compare-locales $(CURDIR)
	chmod +x $(CURDIR)/compare-locales/scripts/*

make-langpack-xpis: debian/stamp-make-langpack-xpis
debian/stamp-make-langpack-xpis: compare-locales/scripts/compare-locales
	@echo ""
	@echo "********************************"
	@echo "* Building language pack xpi's *"
	@echo "********************************"
	@echo ""

	rm -rf $(CURDIR)/debian/l10n-mergedirs
	mkdir $(CURDIR)/debian/l10n-mergedirs

	@export PATH=$(CURDIR)/compare-locales/scripts/:$$PATH ; \
	export PYTHONPATH=$(CURDIR)/compare-locales/lib ; \
	cd $(MOZ_OBJDIR)/$(MOZ_APP)/locales ; \
	while read line ; \
	do \
		line=`echo $$line | sed 's/#.*//' | sed '/^$$/d'` ; \
		if [ ! -z "$$line" ] ; \
		then \
			language=`echo $$line | sed 's/\([^:]*\):*\([^:]*\)/\1/'` ; \
			echo "" ; \
			echo "" ; \
			echo "* Building $${language}" ; \
			echo "" ; \
			$(MAKE) merge-$$language LOCALE_MERGEDIR=$(CURDIR)/debian/l10n-mergedirs/$$language || exit 1 ; \
			$(MAKE) langpack-$$language LOCALE_MERGEDIR=$(CURDIR)/debian/l10n-mergedirs/$$language || exit 1; \
		fi \
	done < $(CURDIR)/debian/config/locales.shipped

	touch $@

common-build-arch:: run-tests $(pkgconfig_files) make-langpack-xpis

common-install-arch common-install-indep::
	$(foreach dir,$(MOZ_LIBDIR) $(MOZ_INCDIR) $(MOZ_IDLDIR) $(MOZ_SDKDIR), \
		if [ -d debian/tmp/$(dir)-$(MOZ_VERSION) ]; \
		then \
			mv debian/tmp/$(dir)-$(MOZ_VERSION) debian/tmp/$(dir); \
		fi; )

common-binary-arch:: make-buildsymbols

binary-install/$(MOZ_PKG_NAME)::
	install -m 0644 $(CURDIR)/debian/apport/blacklist $(CURDIR)/debian/$(MOZ_PKG_NAME)/etc/apport/blacklist.d/$(MOZ_PKG_NAME)
	install -m 0644 $(CURDIR)/debian/apport/native-origins $(CURDIR)/debian/$(MOZ_PKG_NAME)/etc/apport/native-origins.d/$(MOZ_PKG_NAME)

ifeq (1, $(MOZ_ENABLE_GLOBALMENU))
binary-install/$(MOZ_PKG_NAME)-globalmenu::
	unzip -o -d debian/$(MOZ_PKG_NAME)-globalmenu/$(MOZ_ADDONDIR)/extensions/globalmenu@ubuntu.com/ $(MOZ_DISTDIR)/xpi-stage/globalmenu.xpi
	find debian/$(MOZ_PKG_NAME)-globalmenu/$(MOZ_ADDONDIR)/extensions/globalmenu@ubuntu.com/ -type f -executable | xargs chmod -x
endif

GNOME_SUPPORT_FILES = libmozgnome.so

binary-post-install/$(MOZ_PKG_NAME)::
	$(foreach file,$(GNOME_SUPPORT_FILES),rm -fv debian/$(MOZ_PKG_NAME)/$(MOZ_LIBDIR)/components/$(file);) true

binary-post-install/$(MOZ_PKG_NAME)-dev::
	rm -f debian/$(MOZ_PKG_NAME)-dev/$(MOZ_INCDIR)/nspr/md/_linux.cfg
	dh_link -p$(MOZ_PKG_NAME)-dev $(MOZ_INCDIR)/nspr/prcpucfg.h $(MOZ_INCDIR)/nspr/md/_linux.cfg

common-binary-post-install-arch::
	@echo ""
	@echo "**********************************"
	@echo "* Installing language pack xpi's *"
	@echo "**********************************"
	@echo ""

	@while read line ; \
	do \
		line=`echo $$line | sed 's/#.*//' | sed '/^$$/d'` ; \
		if [ ! -z "$$line" ] ; \
		then \
			language=`echo $$line | sed 's/\([^:]*\):*\([^:]*\)/\1/'` ; \
			pkgname=`echo $$line | sed 's/\([^:]*\):*\([^:]*\)/\2/'` ; \
			id=`python $(CURDIR)/debian/build/get-xpi-id.py $(CURDIR)/$(MOZ_DISTDIR)/$(LANGPACK_DIR)/$(MOZ_APP_NAME)-$(MOZ_VERSION).$${language}.langpack.xpi` ; \
			[ $$? -eq 0 ] || exit 1 ; \
			echo "Installing $(MOZ_APP_NAME)-$(MOZ_VERSION).$${language}.langpack.xpi to $${id}.xpi in $(MOZ_PKG_NAME)-locale-$${pkgname}" ; \
			dh_installdirs -p$(MOZ_PKG_NAME)-locale-$${pkgname} $(MOZ_ADDONDIR)/extensions ; \
			cp $(CURDIR)/$(MOZ_DISTDIR)/$(LANGPACK_DIR)/$(MOZ_APP_NAME)-$(MOZ_VERSION).$${language}.langpack.xpi \
			  $(CURDIR)/debian/$(MOZ_PKG_NAME)-locale-$${pkgname}/$(MOZ_ADDONDIR)/extensions/$${id}.xpi ; \
			dh_installdirs -p$(MOZ_PKG_NAME)-locale-$${pkgname} $(MOZ_SEARCHPLUGIN_DIR)/locale/$${language} ; \
			cp -r $(CURDIR)/$(MOZ_DISTDIR)/xpi-stage/locale-$${language}/searchplugins/*.xml \
			  $(CURDIR)/debian/$(MOZ_PKG_NAME)-locale-$${pkgname}/$(MOZ_SEARCHPLUGIN_DIR)/locale/$${language}/. ; \
		fi \
	done < $(CURDIR)/debian/config/locales.shipped

binary-predeb/$(MOZ_PKG_NAME)::
	$(foreach lib,libsoftokn3.so libfreebl3.so libnssdbm3.so, \
	        LD_LIBRARY_PATH=debian/$(MOZ_PKG_NAME)/$(MOZ_LIBDIR):$$LD_LIBRARY_PATH \
	        $(MOZ_DISTDIR)/bin/shlibsign -v -i debian/$(MOZ_PKG_NAME)$(MOZ_LIBDIR)/$(lib);)

common-binary-predeb-arch::
	$(foreach file,$(DEBIAN_EXECUTABLES),chmod a+x debian/$(file);)
	# we want the gnome dependencies not to be in the main package at shlibdeps runtime, hence we dont
	# install them at binary-install/* stage, but copy them over _after_ the shlibdeps had been generated
	$(foreach file,$(GNOME_SUPPORT_FILES),mv debian/$(MOZ_PKG_NAME)-gnome-support/$(MOZ_LIBDIR)/components/$(file) debian/$(MOZ_PKG_NAME)/$(MOZ_LIBDIR)/components/;) true

pre-build:: $(pkgname_subst_files) $(appname_subst_files)
	@cp $(CURDIR)/debian/syspref.js $(CURDIR)/debian/$(MOZ_PKG_BASENAME).js

	@mkdir -p $(DEB_SRCDIR)/extensions/globalmenu
	@(cd debian/globalmenu && tar -cvhf - .) | (cd $(DEB_SRCDIR)/extensions/globalmenu && tar -xf -)
ifeq (,$(MOZ_DEFAULT_APP_BASENAME))
	$(error "Need to set MOZ_DEFAULT_APP_BASENAME")
endif
ifeq (,$(MOZ_BRANDING_OPTION))
	$(error "Need to set MOZ_BRANDING_OPTION")
endif
ifeq (,$(MOZ_BRANDING_DIR))
	$(error "Need to set MOZ_BRANDING_DIR")
endif

refresh-supported-locales: real-refresh-supported-locales debian/control

real-refresh-supported-locales:
	@echo ""
	@echo "****************************************"
	@echo "* Refreshing list of shipped languages *"
	@echo "****************************************"
	@echo ""

	@if [ ! -f $(CURDIR)/upstream-shipped-locales ] ; \
	then \
		if [ ! -z $(TARBALL) ] ; \
		then \
			tarball_opts="-o $(TARBALL)" ; \
		else \
			tarball_opts="" ; \
		fi ; \
		python debian/build/extract-file.py -d $(CURDIR) $$tarball_opts upstream-shipped-locales ; \
		touch $(CURDIR)/upstream-shipped-locales.stamp ; \
	fi
ifdef LANGPACK_O_MATIC
	@perl debian/build/refresh-supported-locales.pl -s $(CURDIR)/upstream-shipped-locales -l $(LANGPACK_O_MATIC) -o $(CURDIR)/debian/config -b $(CURDIR)/debian/config/locales.blacklist
else
	@perl debian/build/refresh-supported-locales.pl -s $(CURDIR)/upstream-shipped-locales -o $(CURDIR)/debian/config -b $(CURDIR)/debian/config/locales.blacklist
endif
	@if [ -f $(CURDIR)/upstream-shipped-locales.stamp ] ; \
	then \
		rm -f $(CURDIR)/upstream-shipped-locales $(CURDIR)/upstream-shipped-locales.stamp ; \
	fi
	

pre-auto-refresh-supported-locales:
	cp debian/config/locales.shipped debian/config/locales.shipped.old

auto-refresh-supported-locales: pre-auto-refresh-supported-locales real-refresh-supported-locales
	@if ! cmp -s debian/config/locales.shipped debian/config/locales.shipped.old ; \
	then \
		echo "" ; \
		echo "****************************************************************************" ; \
		echo "* List of shipped locales is out of date. Please refresh and try again     *" ; \
		echo "* To refresh, run \"debian/rules refresh-supported-locales\" in the source   *" ; \
		echo "* directory. If you are in bzr, you will need to pass the location of the  *" ; \
		echo "* upstream tarball, using \"TARBALL=/path/to/tarball\". If extra information *" ; \
		echo "* is required for new locales, you will also need to pass the location of  *" ; \
		echo "* langpack-o-matic, using \"LANGPACK_O_MATIC=/path/to/langpack-o-matic\"     *" ; \
		echo "****************************************************************************" ; \
		echo "" ; \
		rm -f debian/config/locales.shipped.old ; \
		exit 1 ; \
	fi
	rm -f debian/config/locales.shipped.old

pre-auto-update-debian-control:
	cp debian/control debian/control.old
	touch debian/control.in

post-auto-update-debian-control:
	@if ! cmp -s debian/control debian/control.old ; \
	then \
		echo "" ; \
		echo "*************************************************************************" ; \
		echo "* debian/control file is out of date. Please refresh and try again      *" ; \
		echo "* To refresh, run \"debian/rules debian/control\" in the source directory *" ; \
		echo "*************************************************************************" ; \
		echo "" ; \
		rm -f debian/control.old ; \
		exit 1 ; \
	fi
	rm -f debian/control.old

ifdef PATCHES_DIST
CODENAME = $(PATCHES_DIST)
else
CODENAME = $(DISTRIB_CODENAME)
endif

ifdef PATCHES_ARCH
ARCH = $(PATCHES_ARCH)
else
ARCH = $(DEB_HOST_ARCH)
endif

enable-dist-patches:
	perl $(CURDIR)/debian/build/enable-dist-patches.pl $(CODENAME) $(ARCH) $(CURDIR)/debian/patches/series

ifeq (1, $(NO_AUTO_REFRESH_LOCALES))
clean:: post-auto-update-debian-control
else
clean:: auto-refresh-supported-locales post-auto-update-debian-control
endif
	perl $(CURDIR)/debian/build/enable-dist-patches.pl --clean $(CURDIR)/debian/patches/series
	rm -f $(pkgname_subst_files) $(appname_subst_files)
	rm -f debian/stamp-*
	rm -rf debian/l10n-mergedirs
	rm -rf compare-locales
	rm -f debian/$(MOZ_PKG_BASENAME).js