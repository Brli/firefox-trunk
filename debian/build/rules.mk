#!/usr/bin/make -f

# We need this to execute before the debian/control target gets called
# and before patches are unapplied
clean::
ifneq (1, $(MOZ_DISABLE_CLEAN_CHECKS))
	cp debian/control debian/control.old
	touch debian/control.in
else
	touch debian/control
endif

-include /usr/share/cdbs/1/rules/debhelper.mk
-include /usr/share/cdbs/1/rules/patchsys-quilt.mk
-include /usr/share/cdbs/1/class/makefile.mk

MOZ_OBJDIR		:= $(DEB_BUILDDIR)/obj-$(DEB_HOST_GNU_TYPE)
MOZ_DISTDIR		:= $(MOZ_OBJDIR)/dist

MOZ_BUILDID := $(shell cat $(DEB_SRCDIR)/BUILDID)
$(info Build ID: $(MOZ_BUILDID))

ifeq (,$(MOZ_APP))
$(error "Need to set MOZ_APP")
endif
ifeq (,$(MOZ_APP_NAME))
$(error "Need to set MOZ_APP_NAME")
endif
ifeq (,$(MOZ_PKG_NAME))
$(error "Need to set MOZ_PKG_NAME")
endif
ifeq (,$(MOZ_PKG_BASENAME))
$(error "Need to set MOZ_PKG_BASENAME")
endif
ifeq (,$(MOZ_BRANDING_OPTION))
$(error "Need to set MOZ_BRANDING_OPTION")
endif
ifeq (,$(MOZ_BRANDING_DIR))
$(error "Need to set MOZ_BRANDING_DIR")
endif

DEB_BUILD_DIR			:= $(MOZ_OBJDIR)
# Without this, CDBS passes CFLAGS and CXXFLAGS options to client.mk, which breaks the build
DEB_MAKE_EXTRA_ARGS		:=
# These normally come from autotools.mk, which we no longer include (because we
# don't want to run configure)
DEB_MAKE_INSTALL_TARGET	:=
# Prevent some files from being cleaned to avoid build failures
DEB_CLEAN_EXCLUDE		:= Cargo.toml.orig
# Don't save debug symbols in firefox-dbg (rely on pkg-create-dbgsym to create
# ddeb packages for us). This is needed as long as there is a firefox-dbg
# transitional package
DEB_DH_STRIP_ARGS		:= --dbg-package=$(MOZ_PKG_NAME)-dbg
# We don't want build-tree/mozilla/README to be shipped as a doc
DEB_INSTALL_DOCS_ALL 	:= $(NULL)
# Stop the buildd from timing out during long links
MAKE					:= python3 $(CURDIR)/debian/build/keepalive-wrapper.py 1440 $(MAKE)

MOZ_VERSION		:= $(shell cat $(DEB_SRCDIR)/$(MOZ_APP)/config/version.txt)
MOZ_LIBDIR		:= usr/lib/$(MOZ_APP_NAME)
MOZ_INCDIR		:= usr/include/$(MOZ_APP_NAME)
MOZ_IDLDIR		:= usr/share/idl/$(MOZ_APP_NAME)
MOZ_SDKDIR		:= usr/lib/$(MOZ_APP_NAME)-devel
MOZ_ADDONDIR	:= usr/lib/$(MOZ_APP_NAME)-addons

MOZ_APP_SUBDIR	?=

# If we change MOZ_APP_NAME, we want to set the profile directory
# to match
ifneq (,$(MOZ_VENDOR))
PROFILE_BASE = $(shell echo $(MOZ_VENDOR) | tr A-Z a-z)/
endif
ifneq ($(MOZ_APP_NAME),$(MOZ_DEFAULT_APP_NAME))
MOZ_APP_PROFILE	:= $(PROFILE_BASE)$(MOZ_APP_NAME)
endif

DEB_AUTO_UPDATE_DEBIAN_CONTROL	= no

VIRTENV_PATH	:= $(CURDIR)/$(MOZ_OBJDIR)/_virtualenv
MOZ_PYTHON		:= $(VIRTENV_PATH)/bin/python3
DISTRIB 		:= $(shell lsb_release -i -s)

CFLAGS			:= $(shell echo $(CFLAGS) | sed -e 's/\-g//' | sed -e 's/\-O[s0123]//')
CXXFLAGS		:= $(shell echo $(CFLAGS) | sed -e 's/\-g//' | sed -e 's/\-O[s0123]//')
LDFLAGS			:= $(shell echo $(LDFLAGS) | sed -e 's/-Wl,-Bsymbolic-functions//')

# enable the crash reporter only on i386 and amd64
ifeq (,$(filter i386 amd64,$(DEB_HOST_ARCH)))
MOZ_ENABLE_BREAKPAD = 0
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

ifeq (,$(filter i386 amd64, $(DEB_HOST_ARCH)))
MOZ_BUILD_PGO = 0
endif

LLVM_VERSIONS = 15 14 13 12 10
DEB_LLVM_VERSION = $(firstword $(foreach llvm_version, $(LLVM_VERSIONS), \
	$(if $(shell which clang-$(llvm_version)), $(llvm_version))))

export HOME=/tmp
export SHELL=/bin/bash
export NO_PNG_PKG_MANGLE=1
export MACH_BUILD_PYTHON_NATIVE_PACKAGE_SOURCE=none
export MOZBUILD_STATE_PATH=/tmp/.mozbuild

ifeq (linux-gnu, $(DEB_HOST_GNU_SYSTEM))
LANGPACK_DIR := linux-$(DEB_HOST_GNU_CPU)/xpi
else
LANGPACK_DIR := $(DEB_HOST_GNU_SYSTEM)-$(DEB_HOST_GNU_CPU)/xpi
endif

MOZ_PKG_SUPPORT_SUGGESTS ?=

# Defines used for the Mozilla text preprocessor
MOZ_DEFINES += 	-DMOZ_LIBDIR="$(MOZ_LIBDIR)" -DMOZ_APP_NAME="$(MOZ_APP_NAME)" -DMOZ_BUILDID="$(MOZ_BUILDID)" \
		-DMOZ_INCDIR="$(MOZ_INCDIR)" -DMOZ_IDLDIR="$(MOZ_IDLDIR)" -DMOZ_VERSION="$(MOZ_VERSION)" -DDEB_HOST_ARCH="$(DEB_HOST_ARCH)" \
		-DMOZ_DISPLAY_NAME="$(MOZ_DISPLAY_NAME)" -DMOZ_PKG_NAME="$(MOZ_PKG_NAME)" -DDISTRIB="$(DISTRIB)" \
		-DMOZ_BRANDING_OPTION="$(MOZ_BRANDING_OPTION)" -DTOPSRCDIR="$(CURDIR)" -DDEB_HOST_GNU_TYPE="$(DEB_HOST_GNU_TYPE)" \
		-DMOZ_ADDONDIR="$(MOZ_ADDONDIR)" -DMOZ_SDKDIR="$(MOZ_SDKDIR)" -DMOZ_DISTDIR="$(MOZ_DISTDIR)" -DMOZ_UPDATE_CHANNEL="$(CHANNEL)" \
		-DMOZ_OBJDIR="$(MOZ_OBJDIR)" -DDEB_BUILDDIR="$(DEB_BUILDDIR)" -DMOZ_PYTHON="$(MOZ_PYTHON)" -DDEB_BUILD_ARCH_BITS=$(DEB_BUILD_ARCH_BITS) \
		-DMOZ_DEFAULT_APP_NAME="$(MOZ_DEFAULT_APP_NAME)" -DDISTRIB_VERSION="$(DISTRIB_VERSION_MAJOR)$(DISTRIB_VERSION_MINOR)" \
		-DDEB_LLVM_VERSION="$(DEB_LLVM_VERSION)"

ifneq (,$(MOZ_APP_PROFILE))
MOZ_DEFINES += -DMOZ_APP_PROFILE="$(MOZ_APP_PROFILE)"
endif
ifeq (1, $(MOZ_ENABLE_BREAKPAD))
MOZ_DEFINES += -DMOZ_ENABLE_BREAKPAD
endif
ifeq (1, $(MOZ_VALGRIND))
MOZ_DEFINES += -DMOZ_VALGRIND
endif
ifeq (1,$(MOZ_NO_OPTIMIZE))
MOZ_DEFINES += -DMOZ_NO_OPTIMIZE
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
ifeq (official, $(MOZ_BRANDING))
MOZ_DEFINES += -DMOZ_OFFICIAL_BRANDING
endif
ifneq (,$(DEB_PARALLEL_JOBS))
MOZ_DEFINES += -DDEB_PARALLEL_JOBS=$(DEB_PARALLEL_JOBS)
endif

MOZ_EXECUTABLES_$(MOZ_PKG_NAME) +=	$(MOZ_LIBDIR)/$(MOZ_PKG_BASENAME).sh \
					$(NULL)

pkgname_subst_files = \
	debian/config/mozconfig \
	$(MOZ_PKGNAME_SUBST_FILES) \
	$(NULL)

$(foreach pkg,$(DEB_ALL_PACKAGES), \
	$(foreach e,install dirs links manpages postinst preinst postrm prerm lintian-overrides,\
		$(if $(wildcard debian/$(subst $(MOZ_PKG_NAME),$(MOZ_PKG_BASENAME),$(pkg)).$(e).in),\
			$(eval pkgname_subst_files += debian/$(pkg).$(e)))))

appname_subst_files = \
	debian/$(MOZ_APP_NAME).desktop \
	$(MOZ_APPNAME_SUBST_FILES) \
	$(NULL)

debian/control:: debian/control.in debian/control.langpacks debian/control.langpacks.unavail debian/config/locales.shipped debian/config/locales.all
	@echo ""
	@echo "*****************************"
	@echo "* Refreshing debian/control *"
	@echo "*****************************"
	@echo ""

	cp debian/control.in debian/control.tmp
	perl debian/build/dump-langpack-control-entries.pl >> debian/control.tmp
	sed -e 's/@MOZ_PKG_NAME@/$(MOZ_PKG_NAME)/g' < debian/control.tmp > debian/control
	rm -f debian/control.tmp

	sed -i -e 's/@MOZ_LOCALE_PKGS@/$(foreach p,$(MOZ_LOCALE_PKGS),$(p) \(= $${binary:Version}\),)/' debian/control

$(pkgname_subst_files): $(foreach file,$(pkgname_subst_files),$(subst $(MOZ_PKG_NAME),$(MOZ_PKG_BASENAME),$(file).in))
	PYTHONDONTWRITEBYTECODE=1 python3 $(CURDIR)/debian/build/Preprocessor.py -Fsubstitution --marker="%%" $(MOZ_DEFINES) $(CURDIR)/$(subst $(MOZ_PKG_NAME),$(MOZ_PKG_BASENAME),$@.in) > $(CURDIR)/$@

$(appname_subst_files): $(foreach file,$(appname_subst_files),$(subst $(MOZ_APP_NAME),$(MOZ_PKG_BASENAME),$(file).in))
	PYTHONDONTWRITEBYTECODE=1 python3 $(CURDIR)/debian/build/Preprocessor.py -Fsubstitution --marker="%%" $(MOZ_DEFINES) $(CURDIR)/$(subst $(MOZ_APP_NAME),$(MOZ_PKG_BASENAME),$@.in) > $(CURDIR)/$@

%.pc: WCHAR_CFLAGS = $(shell cat $(MOZ_OBJDIR)/config/autoconf.mk | grep WCHAR_CFLAGS | sed 's/^[^=]*=[[:space:]]*\(.*\)$$/\1/')
%.pc: %.pc.in debian/stamp-makefile-build
	PYTHONDONTWRITEBYTECODE=1 python3 $(CURDIR)/debian/build/Preprocessor.py -Fsubstitution --marker="%%" $(MOZ_DEFINES) -DWCHAR_CFLAGS="$(WCHAR_CFLAGS)" $(CURDIR)/$< > $(CURDIR)/$@

make-buildsymbols: debian/stamp-makebuildsymbols
debian/stamp-makebuildsymbols: debian/stamp-makefile-build
	$(MAKE) -C $(MOZ_OBJDIR) buildsymbols MOZ_SYMBOLS_EXTRA_BUILDID=$(shell date -d "`dpkg-parsechangelog | grep Date: | sed -e 's/^Date: //'`" +%y%m%d%H%M%S)-$(DEB_HOST_GNU_CPU)
	@touch $@

install-geckodriver: debian/stamp-installgeckodriver
debian/stamp-installgeckodriver: debian/stamp-makefile-install
	install -D $(MOZ_DISTDIR)/bin/geckodriver $(CURDIR)/debian/$(MOZ_PKG_NAME)-geckodriver/usr/bin/geckodriver
	@touch $@

make-langpack-xpis: $(foreach locale,$(MOZ_LOCALES),debian/stamp-make-langpack-xpi-$(locale))
debian/stamp-make-langpack-xpi-%:
	@echo ""
	@echo ""
	@echo "* Building language pack xpi for $*"
	@echo ""

	export PATH=$(VIRTENV_PATH)/bin/:$$PATH ; \
	export REAL_LOCALE_MERGEDIR=$(CURDIR)/debian/l10n-mergedirs/$* ; \
	cd $(MOZ_OBJDIR)/$(MOZ_APP)/locales ; \
		$(MAKE) langpack-$* BASE_MERGE=$(CURDIR)/debian/l10n-mergedirs REAL_LOCALE_MERGEDIR=$(CURDIR)/debian/l10n-mergedirs/$* || exit 1;
	@touch $@

common-configure-arch common-configure-indep:: common-configure-impl
common-configure-impl:: debian/stamp-mach-configure
debian/stamp-mach-configure: cbindgen/bin/cbindgen dump_syms/bin/dump_syms
	$(CURDIR)/mach configure && $(CURDIR)/mach build-backend
	touch $@
clean::
	rm -f debian/stamp-mach-configure

cbindgen/bin/cbindgen: third_party/cbindgen/Cargo.toml
	export RUST_BACKTRACE=full; \
	export CC=clang-$(DEB_LLVM_VERSION); \
	export CXX=clang++-$(DEB_LLVM_VERSION); \
	cd $(CURDIR)/third_party/cbindgen; \
	cargo build --release; \
	export CARGO_HOME=$(CURDIR)/third_party/cbindgen/.cargo; \
	cargo install --path . --bin cbindgen --root ../../cbindgen
clean::
	rm -rf $(CURDIR)/cbindgen
	rm -rf $(CURDIR)/third_party/cbindgen/target

dump_syms/bin/dump_syms: third_party/dump_syms/Cargo.toml
	export RUST_BACKTRACE=full; \
	export CC=clang-$(DEB_LLVM_VERSION); \
	export CXX=clang++-$(DEB_LLVM_VERSION); \
	cd $(CURDIR)/third_party/dump_syms; \
	cargo build --release; \
	export CARGO_HOME=$(CURDIR)/third_party/dump_syms/.cargo; \
	cargo install --path . --bin dump_syms --root ../../dump_syms
clean::
	rm -rf $(CURDIR)/dump_syms
	rm -rf $(CURDIR)/third_party/dump_syms/target

install/$(MOZ_PKG_NAME)::
	@echo "Adding suggests / recommends on support packages"
	echo "$(MOZ_PKG_SUPPORT_SUGGESTS)" | perl -0 -ne 's/[ \t\n]+/ /g; /\w/ and print "support:Suggests=$$_\n"' >> debian/$(MOZ_PKG_NAME).substvars
	echo "$(MOZ_PKG_SUPPORT_RECOMMENDS)" | perl -0 -ne 's/[ \t\n]+/ /g; /\w/ and print "support:Recommends=$$_\n"' >> debian/$(MOZ_PKG_NAME).substvars

ifneq ($(MOZ_PKG_NAME),$(MOZ_APP_NAME))
install/%::
	@echo "Adding conflicts / provides for renamed package"
	echo "app:Conflicts=$(subst $(subst $(MOZ_APP_NAME),,$(MOZ_PKG_NAME)),,$*)" >> debian/$*.substvars
	echo "app:Provides=$(subst $(subst $(MOZ_APP_NAME),,$(MOZ_PKG_NAME)),,$*)" >> debian/$*.substvars
endif

common-install-arch common-install-indep:: common-install-impl
common-install-impl:: debian/stamp-mach-install make-langpack-xpis
debian/stamp-mach-install:
	DESTDIR=$(CURDIR)/debian/tmp $(CURDIR)/mach install
	$(foreach dir,$(MOZ_LIBDIR) $(MOZ_INCDIR) $(MOZ_IDLDIR) $(MOZ_SDKDIR), \
		if [ -d debian/tmp/$(dir)-$(MOZ_VERSION) ]; \
		then \
			mv debian/tmp/$(dir)-$(MOZ_VERSION) debian/tmp/$(dir); \
		fi; )
	touch $@
clean::
	rm -f debian/stamp-mach-install

common-install-arch:: install-geckodriver

common-binary-arch:: make-buildsymbols

binary-install/$(MOZ_PKG_NAME)::
	install -m 0644 $(CURDIR)/debian/apport/blacklist $(CURDIR)/debian/$(MOZ_PKG_NAME)/etc/apport/blacklist.d/$(MOZ_PKG_NAME)
	install -m 0644 $(CURDIR)/debian/apport/native-origins $(CURDIR)/debian/$(MOZ_PKG_NAME)/etc/apport/native-origins.d/$(MOZ_PKG_NAME)
	# Copy hicolor icons (LP: #1639863)
	$(foreach size,16 22 24 32 48 64 128 256, \
		install -m 0644 -D $(CURDIR)/browser/branding/nightly/default$(size).png \
			$(CURDIR)/debian/$(MOZ_PKG_NAME)/usr/share/icons/hicolor/$(size)x$(size)/apps/$(MOZ_PKG_NAME).png;)
	# Monochrome/symbolic icon for gnome-shell
	install -m 0644 $(CURDIR)/debian/symbolic.svg $(CURDIR)/debian/$(MOZ_PKG_NAME)/usr/share/icons/hicolor/symbolic/apps/$(MOZ_PKG_NAME)-symbolic.svg

$(patsubst %,binary-post-install/%,$(MOZ_LOCALE_PKGS)):: binary-post-install/%: install-langpack-xpis-%

binary-post-install/$(MOZ_PKG_NAME)-dev::
	rm -f debian/$(MOZ_PKG_NAME)-dev/$(MOZ_INCDIR)/nspr/md/_linux.cfg
	dh_link -p$(MOZ_PKG_NAME)-dev $(MOZ_INCDIR)/nspr/prcpucfg.h $(MOZ_INCDIR)/nspr/md/_linux.cfg

$(patsubst %,binary-post-install/%,$(DEB_ALL_PACKAGES)) :: binary-post-install/%:
	find debian/$(cdbs_curpkg) -name .mkdir.done -delete

define locales_for_langpack
$(strip $(if $(filter $(MOZ_PKG_NAME),$(1)),\
	en-US,\
	$(shell grep $(subst $(MOZ_PKG_NAME)-locale-,,$(1))$$ debian/config/locales.shipped | sed -n 's/\([^\:]*\)\:\?.*/\1/ p')))
endef

install-langpack-xpis-%:
	@echo ""
	@echo "Installing language pack xpis for $*"
	dh_installdirs -p$* $(MOZ_ADDONDIR)/extensions
	$(foreach lang,$(call locales_for_langpack,$*), \
		id=`PYTHONDONTWRITEBYTECODE=1 python3 $(CURDIR)/debian/build/xpi-id.py $(CURDIR)/$(MOZ_DISTDIR)/$(LANGPACK_DIR)/$(MOZ_APP_NAME)-$(MOZ_VERSION).$(lang).langpack.xpi 2>/dev/null`; \
		install -m 0644 $(CURDIR)/$(MOZ_DISTDIR)/$(LANGPACK_DIR)/$(MOZ_APP_NAME)-$(MOZ_VERSION).$(lang).langpack.xpi \
			$(CURDIR)/debian/$*/$(MOZ_ADDONDIR)/extensions/$$id.xpi;)

$(patsubst %,binary-fixup/%,$(DEB_ALL_PACKAGES)) :: binary-fixup/%:
	find debian/$(cdbs_curpkg) -type f -perm -5 \( -name '*.zip' -or -name '*.xml' -or -name '*.js' -or -name '*.manifest' -or -name '*.xpt' \) -print0 2>/dev/null | xargs -0r chmod 644
	$(foreach f,$(call cdbs_expand_curvar,MOZ_EXECUTABLES),chmod a+x debian/$(cdbs_curpkg)/$(f);)

mozconfig: debian/config/mozconfig
	cp $< $@

define cmp_auto_generated_file
@if ! cmp -s $(1) $(1).old; then \
	echo ""; \
	diff -Nurp $(1).old $(1); \
	echo ""; \
	echo "****************************************************************************"; \
	echo "* An automatically generated file is out of date and needs to be refreshed *"; \
	echo "****************************************************************************"; \
	echo ""; \
	echo "$(1) is out of date. Please run \"debian/rules $(firstword $(2) $(1))\" in VCS"; \
	echo ""; \
	rm -f $(1).old; \
	exit 1; \
fi
rm -f $(1).old
endef

pre-build::
	cp debian/config/locales.shipped debian/config/locales.shipped.old
pre-build:: debian/config/locales.shipped $(pkgname_subst_files) $(appname_subst_files) mozconfig
	$(call cmp_auto_generated_file,debian/config/locales.shipped,refresh-supported-locales)

# Conditionally patch the top-level Cargo.toml file to reduce the LTO to "thin"
# on armhf to work around OOM failures on Launchpad builders. This is only one
# half of the workaround, see also debian/patches/armhf-rustc-thin-lto.patch.
ifneq (,$(filter arm64 armhf i386, $(DEB_HOST_ARCH)))
pre-build:: Cargo.toml.bak
Cargo.toml.bak: Cargo.toml
	cp $< $@
	sed -i 's/\[profile.release\]/\[profile.release\]\nlto = "thin"/' $<
clean::
	if [ -f Cargo.toml.bak ]; then mv Cargo.toml.bak Cargo.toml; fi
endif

EXTRACT_TARBALL = $(firstword $(shell TMPDIR=`mktemp -d`; tar -jxf $(1) -C $$TMPDIR > /dev/null 2>&1; echo $$TMPDIR/`ls $$TMPDIR/ | head -n1`))

ifdef LANGPACK_O_MATIC
refresh-supported-locales:: LPOM_OPT = -l $(LANGPACK_O_MATIC)
endif
refresh-supported-locales:: EXTRACTED := $(if $(wildcard $(MOZ_APP)/locales/shipped-locales),,$(call EXTRACT_TARBALL,$(TARBALL)))
refresh-supported-locales:: SHIPPED_LOCALES = $(firstword $(wildcard $(CURDIR)/$(MOZ_APP)/locales/shipped-locales) $(wildcard $(EXTRACTED)/$(MOZ_APP)/locales/shipped-locales))
refresh-supported-locales::
	@echo ""
	@echo "****************************************"
	@echo "* Refreshing list of shipped languages *"
	@echo "****************************************"
	@echo ""

	$(if $(SHIPPED_LOCALES),,$(error We aren't in the full source directory. Please use "TARBALL=<path_to_orig.tar.bzr>"))

	perl debian/build/refresh-supported-locales.pl -s $(SHIPPED_LOCALES) $(LPOM_OPT)

refresh-supported-locales:: debian/control
	$(if $(EXTRACTED),rm -rf $(dir $(EXTRACTED)))

define moz_monkey_patch_file
$(if $(wildcard debian/stamp-monkey-patch-upstream-files),$(error Too late to use moz_monkey_patch_file), \
	echo "$(1)" >> debian/monkey-patch-files; \
	echo "$(2) $(1)" >> debian/monkey-patch-files.sh)
endef

get-orig-source: ARGS = -r $(MOZILLA_REPO) -l $(L10N_REPO) -n $(MOZ_PKG_NAME) -b $(MOZ_PKG_BASENAME) -a $(MOZ_APP)
ifdef UPSTREAM_VERSION
get-orig-source: ARGS += -v $(UPSTREAM_VERSION)
endif
ifdef UPSTREAM_BUILD
get-orig-source: ARGS += --build $(UPSTREAM_BUILD)
endif
ifdef LOCAL_BRANCH
get-orig-source: ARGS += -c $(LOCAL_BRANCH)
endif
get-orig-source:
	PYTHONDONTWRITEBYTECODE=1 python3 $(CURDIR)/debian/build/create-tarball.py $(ARGS)

echo-%:
	@echo "$($*)"

ifneq (1, $(MOZ_DISABLE_CLEAN_CHECKS))
clean::
	cp debian/config/locales.shipped debian/config/locales.shipped.old
clean:: refresh-supported-locales
	$(call cmp_auto_generated_file,debian/config/locales.shipped,refresh-supported-locales)
	$(call cmp_auto_generated_file,debian/control)
endif

clean::
	rm -f $(pkgname_subst_files) $(appname_subst_files)
	rm -f debian/stamp-*
	rm -rf debian/l10n-mergedirs
	rm -rf $(MOZ_OBJDIR)
	rm -f mozconfig

.PHONY: make-buildsymbols make-langpack-xpis refresh-supported-locales get-orig-source monkey-patch-upstream-files
