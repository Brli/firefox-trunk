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

# We need this to execute before the debian/control target gets called
clean:: pre-auto-update-debian-control

-include /usr/share/cdbs/1/rules/debhelper.mk
-include /usr/share/cdbs/1/rules/patchsys-quilt.mk
-include /usr/share/cdbs/1/class/makefile.mk
include $(CURDIR)/debian/build/get-orig-source.mk

MOZ_OBJDIR		:= $(DEB_BUILDDIR)/obj-$(DEB_HOST_GNU_TYPE)
MOZ_DISTDIR		:= $(MOZ_OBJDIR)/$(MOZ_MOZDIR)/dist

# Define other variables used throughout the build
# The value of "Name" to use in application.ini, which the profile location is based on.
# Derived from the desired MOZ_APP_NAME, but can be overridden
MOZ_APP_BASENAME	?= $(shell echo $(MOZ_APP_NAME) | sed -n 's/\-.\|\<./\U&/g p')
# The default value of "Name" in the application.ini, derived from the upstream build system
# It is used for the profile location. This should be set manually if not provided
MOZ_DEFAULT_APP_BASENAME ?= $(shell . ./$(DEB_SRCDIR)/$(MOZ_APP)/confvars.sh; echo $$MOZ_APP_BASENAME)
# Equal to upstreams default MOZ_APP_NAME. If not a lower case version of the "Name"
# in application.ini, then this should be manually overridden
MOZ_DEFAULT_APP_NAME	?= $(MOZ_DEFAULT_APP_BASENAME_L)
# Location for searchplugins
MOZ_SEARCHPLUGIN_DIR	?= $(MOZ_LIBDIR)/distribution/searchplugins

MOZ_APP_BASENAME_L	:= $(shell echo $(MOZ_APP_BASENAME) | tr A-Z a-z)
MOZ_DEFAULT_APP_BASENAME_L := $(shell echo $(MOZ_DEFAULT_APP_BASENAME) | tr A-Z a-z)

ifeq (,$(MOZ_APP))
$(error "Need to set MOZ_APP")
endif
ifeq (,$(MOZ_APP_NAME))
$(error "Need to set MOZ_APP_NAME")
endif
ifeq (,$(MOZ_PKG_NAME))
$(error "Need to set MOZ_PKG_NAME")
endif

DEB_MAKE_MAKEFILE	:= client.mk
# Without this, CDBS passes CFLAGS and CXXFLAGS options to client.mk, which breaks the build
DEB_MAKE_EXTRA_ARGS	:=
# These normally come from autotools.mk, which we no longer include (because we
# don't want to run configure)
DEB_MAKE_INSTALL_TARGET	:= install DESTDIR=$(CURDIR)/debian/tmp
DEB_MAKE_CLEAN_TARGET	:= cleansrcdir
DEB_DH_STRIP_ARGS	:= --dbg-package=$(MOZ_PKG_NAME)-dbg
# We don't want build-tree/mozilla/README to be shipped as a doc
DEB_INSTALL_DOCS_ALL 	:= $(NULL)

MOZ_VERSION		:= $(shell cat $(DEB_SRCDIR)/$(MOZ_APP)/config/version.txt)
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

CFLAGS			:= -g
CXXFLAGS		:= -g
LDFLAGS 		:= $(shell echo $$LDFLAGS | sed -e 's/-Wl,-Bsymbolic-functions//')

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
ifeq (,$(filter lucid maverick natty oneiric,$(DISTRIB_CODENAME)))
SUPPORTED_ARM = armhf
else
SUPPORTED_ARM = armel
endif
ifeq (,$(filter i386 amd64 $(SUPPORTED_ARM),$(DEB_HOST_ARCH)))
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

MOZ_DISPLAY_NAME = $(shell cat $(DEB_SRCDIR)/$(MOZ_BRANDING_DIR)/locales/en-US/brand.properties \
		    | grep brandShortName | sed -e 's/brandShortName\=//')

ifeq (,$(filter 4.7, $(shell $(CC) -dumpversion)))
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

ifeq (1,$(MOZ_ENABLE_GLOBALMENU))
ifneq (,$(PKG_SUPPORT_RECOMMENDS))
PKG_SUPPORT_RECOMMENDS += ", "
endif
PKG_SUPPORT_RECOMMENDS += $(MOZ_APP_NAME)-globalmenu
endif

DEB_DH_GENCONTROL_ARGS_$(MOZ_PKG_NAME) := -- -Vapp:Replaces="$(PKG_APP_REPLACES_ARGS)" -Vapp:Breaks="$(PKG_APP_BREAKS_ARGS)" -Vapp:Conflicts="$(PKG_APP_CONFLICTS_ARGS)" \
					     -Vapp:Provides="$(PKG_APP_PROVIDES_ARGS)" $(PKG_APP_EXTRA_ARGS) -Vsupport:Suggests="$(PKG_SUPPORT_SUGGESTS)" \
					     -Vsupport:Recommends="$(PKG_SUPPORT_RECOMMENDS)"
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
LOCALE_PACKAGES := $(shell cat $(CURDIR)/debian/control | grep "^Package:[[:space:]]*$(MOZ_PKG_NAME)-locale\-" | sed -n -e 's/^Package\:[[:space:]]*\([^[:space:]]*\)/\1/ p')
ifneq ($(MOZ_PKG_NAME),$(MOZ_APP_NAME))
$(foreach locale_package, $(LOCALE_PACKAGES), $(eval PKG_$(locale_package)_CONFLICTS_ARGS := $(subst $(MOZ_PKG_NAME),$(MOZ_APP_NAME),$(locale_package))))
$(foreach locale_package, $(LOCALE_PACKAGES), $(eval PKG_$(locale_package)_PROVIDES_ARGS := $(subst $(MOZ_PKG_NAME),$(MOZ_APP_NAME),$(locale_package))))
endif
$(foreach locale_package, $(LOCALE_PACKAGES), $(eval DEB_DH_GENCONTROL_ARGS_$(locale_package) := -- -Vlp:Conflicts="$(PKG_$(locale_package)_PROVIDES_ARGS)" \
												    -Vlp:Provides="$(PKG_$(locale_package)_PROVIDES_ARGS)" \
												    $(PKG_$(locale_package)_EXTRA_ARGS)))

# Defines used for the Mozilla text preprocessor
MOZ_DEFINES += 	-DMOZ_LIBDIR="$(MOZ_LIBDIR)" -DMOZ_APP_NAME="$(MOZ_APP_NAME)" -DMOZ_APP_BASENAME="$(MOZ_APP_BASENAME)" \
		-DMOZ_INCDIR="$(MOZ_INCDIR)" -DMOZ_IDLDIR="$(MOZ_IDLDIR)" -DMOZ_VERSION="$(MOZ_VERSION)" -DDEB_HOST_ARCH="$(DEB_HOST_ARCH)" \
		-DMOZ_DISPLAY_NAME="$(MOZ_DISPLAY_NAME)" -DMOZ_SYSTEM_DICTDIR="$(MOZ_SYSTEM_DICTDIR)" -DMOZ_PKG_NAME="$(MOZ_PKG_NAME)" \
		-DMOZ_BRANDING_OPTION="$(MOZ_BRANDING_OPTION)" -DTOPSRCDIR="$(CURDIR)" -DDEB_HOST_GNU_TYPE="$(DEB_HOST_GNU_TYPE)" \
		-DMOZ_ADDONDIR="$(MOZ_ADDONDIR)" -DMOZ_SDKDIR="$(MOZ_SDKDIR)" -DMOZ_DISTDIR="$(MOZ_DISTDIR)" -DMOZ_UPDATE_CHANNEL="$(CHANNEL)" \
		-DMOZ_OBJDIR="$(MOZ_OBJDIR)" -DDEB_BUILDDIR="$(DEB_BUILDDIR)" -DMOZ_PYTHON="$(MOZ_PYTHON)" -DMOZ_PROFILEDIR="$(MOZ_PROFILEDIR)" \
		-DMOZ_PKG_BASENAME="$(MOZ_PKG_BASENAME)" -DMOZ_DEFAULT_PROFILEDIR="$(MOZ_DEFAULT_PROFILEDIR)" \
		-DMOZ_DEFAULT_APP_NAME="$(MOZ_DEFAULT_APP_NAME)" -DMOZ_DEFAULT_APP_BASENAME="$(MOZ_DEFAULT_APP_BASENAME)"

ifeq (1, $(MOZ_ENABLE_BREAKPAD))
MOZ_DEFINES += -DMOZ_ENABLE_BREAKPAD
endif
ifeq (1, $(MOZ_VALGRIND))
MOZ_DEFINES += -DMOZ_VALGRIND
endif
ifeq (1,$(MOZ_NO_OPTIMIZE))
MOZ_DEFINES += -DMOZ_NO_OPTIMIZE
endif
ifeq (1,$(MOZ_WANT_UNIT_TESTS))
MOZ_DEFINES += -DMOZ_WANT_UNIT_TESTS
endif
ifneq ($(DEB_BUILD_GNU_TYPE),$(DEB_HOST_GNU_TYPE))
MOZ_DEFINES += -DDEB_BUILD_GNU_TYPE="$(DEB_BUILD_GNU_TYPE)"
endif
ifeq (1,$(MOZ_BUILD_PGO))
MOZ_DEFINES += -DMOZ_BUILD_PGO
endif
ifeq (1,$(MOZ_DEBUG))
MOZ_DEFINES += -DMOZ_DEBUG
endif
ifeq (1,$(MOZ_ENABLE_GLOBALMENU))
MOZ_DEFINES += -DMOZ_ENABLE_GLOBALMENU
endif
ifeq (official, $(MOZ_BRANDING))
MOZ_DEFINES += -DMOZ_OFFICIAL_BRANDING
endif
ifeq (,$(filter $(MOZ_OLD_SYSPREF_RELEASES), $(DISTRIB_CODENAME)))
MOZ_DEFINES += -DMOZ_NEW_SYSPREF
endif
ifeq (,$(filter lucid maverick natty oneiric, $(DISTRIB_CODENAME)))
MOZ_DEFINES += -DMOZ_FREEDESKTOP_ACTIONS
endif
ifneq (,$(filter lucid maverick natty oneiric precise, $(DISTRIB_CODENAME)))
ifneq (,$(filter armel,$(DEB_HOST_ARCH)))
MOZ_DEFINES += -DMOZ_ENABLE_THUMB
endif
endif

DEBIAN_EXECUTABLES = $(MOZ_PKG_NAME)/$(MOZ_LIBDIR)/$(MOZ_PKG_BASENAME).sh \
		     $(NULL)

pkgname_subst_files = \
	debian/config/mozconfig \
	$(MOZ_PKGNAME_SUBST_FILES) \
	$(NULL)

$(foreach pkg, $(MOZ_PKG_NAME) $(MOZ_PKG_NAME)-gnome-support $(MOZ_PKG_NAME)-globalmenu $(MOZ_PKG_NAME)-dev $(MOZ_PKG_NAME)-dbg $(MOZ_PKG_NAME)-mozsymbols, \
	$(foreach dhfile, install dirs links manpages postinst preinst postrm prerm lintian-overrides, $(eval pkgname_subst_files += \
	$(shell if [ -f $(CURDIR)/$(subst $(MOZ_PKG_NAME),$(MOZ_PKG_BASENAME),debian/$(pkg).$(dhfile).in) ]; then \
		echo debian/$(pkg).$(dhfile); fi))))

appname_subst_files = \
	debian/$(MOZ_APP_NAME).desktop \
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

$(pkgname_subst_files): $(foreach file,$(pkgname_subst_files),$(subst $(MOZ_PKG_NAME),$(MOZ_PKG_BASENAME),$(file).in))
	$(MOZ_PYTHON) $(CURDIR)/$(DEB_SRCDIR)/$(MOZ_MOZDIR)/config/Preprocessor.py -Fsubstitution --marker="%%" $(MOZ_DEFINES) $(CURDIR)/$(subst $(MOZ_PKG_NAME),$(MOZ_PKG_BASENAME),$@.in) > $(CURDIR)/$@

$(appname_subst_files): $(foreach file,$(appname_subst_files),$(subst $(MOZ_APP_NAME),$(MOZ_PKG_BASENAME),$(file).in))
	$(MOZ_PYTHON) $(CURDIR)/$(DEB_SRCDIR)/$(MOZ_MOZDIR)/config/Preprocessor.py -Fsubstitution --marker="%%" $(MOZ_DEFINES) $(CURDIR)/$(subst $(MOZ_APP_NAME),$(MOZ_PKG_BASENAME),$@.in) > $(CURDIR)/$@

%.pc: WCHAR_CFLAGS = $(shell cat $(MOZ_OBJDIR)/config/autoconf.mk | grep WCHAR_CFLAGS | sed 's/^[^=]*=[[:space:]]*\(.*\)$$/\1/')
%.pc: %.pc.in debian/stamp-makefile-build
	$(MOZ_PYTHON) $(DEB_SRCDIR)/$(MOZ_MOZDIR)/config/Preprocessor.py -Fsubstitution --marker="%%" $(MOZ_DEFINES) -DWCHAR_CFLAGS="$(WCHAR_CFLAGS)" $(CURDIR)/$< > $(CURDIR)/$@

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

pre-build:: $(pkgname_subst_files) $(appname_subst_files) enable-dist-patches
	@cp $(CURDIR)/debian/syspref.js $(CURDIR)/debian/$(MOZ_PKG_BASENAME).js

	@mkdir -p $(DEB_SRCDIR)/$(MOZ_MOZDIR)/extensions/globalmenu
	@(cd debian/globalmenu && tar -cvhf - .) | (cd $(DEB_SRCDIR)/$(MOZ_MOZDIR)/extensions/globalmenu && tar -xf -)

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

	@if [ ! -f $(CURDIR)/$(MOZ_APP)/locales/shipped-locales ] ; \
	then \
		if [ ! -z $(TARBALL) ] ; \
		then \
			python debian/build/extract-file.py -o $(CURDIR)/.upstream-shipped-locales -t $(TARBALL) -i $(MOZ_APP)/locales/shipped-locales ; \
		fi \
	else \
		cp $(CURDIR)/$(MOZ_APP)/locales/shipped-locales $(CURDIR)/.upstream-shipped-locales ; \
	fi
ifdef LANGPACK_O_MATIC
	@perl debian/build/refresh-supported-locales.pl -s $(CURDIR)/.upstream-shipped-locales -l $(LANGPACK_O_MATIC) -o $(CURDIR)/debian/config -b $(CURDIR)/debian/config/locales.blacklist
else
	@perl debian/build/refresh-supported-locales.pl -s $(CURDIR)/.upstream-shipped-locales -o $(CURDIR)/debian/config -b $(CURDIR)/debian/config/locales.blacklist
endif
	@rm -f $(CURDIR)/.upstream-shipped-locales
	

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

RESTORE_BACKUP = $(shell if [ -f $(1).bak ] ; then rm -f $(1); mv $(1).bak $(1); fi)

echo-%:
	@echo "$($*)"

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
	rm -rf $(MOZ_OBJDIR)
