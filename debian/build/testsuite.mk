#!/usr/bin/make -f

LOCALE		:= en_US.UTF-8
LOCDIR		= $(CURDIR)/$(MOZ_DISTDIR)/.locales

ifeq (1,$(MOZ_WANT_UNIT_TESTS))
ifeq (,$(MOZ_TESTS))
TESTS = check jstestbrowser reftest crashtest mochitest
else
TESTS = $(MOZ_TESTS)
endif
endif

run-tests: $(addprefix debian/stamp-,$(TESTS))

$(addprefix debian/stamp-,$(TESTS)): debian/stamp-makefile-build

# Required for js/src/trace-tests/sunspider/check-date-format-tofte.js
$(addprefix debian/stamp-,check jstestbrowser): export TZ = :/usr/share/zoneinfo/posix/US/Pacific

$(LOCDIR)/%:
	mkdir -p $(LOCDIR)
	localedef -f $(shell echo $(notdir $@) | cut -d '.' -f 2) -i $(shell echo $(notdir $@) | cut -d '.' -f 1) $@

# Setup locales for tests which need it
$(addprefix debian/stamp-,jstestbrowser reftest): $(LOCDIR)/$(LOCALE)
$(addprefix debian/stamp-,jstestbrowser reftest): export LOCPATH=$(LOCDIR)
$(addprefix debian/stamp-,jstestbrowser reftest): export LC_ALL=$(LOCALE)

# Tests that need a X server
$(addprefix debian/stamp-,jstestbrowser reftest crashtest mochitest): WRAPPER = xvfb-run -a -s "-screen 0 1024x768x24" dbus-launch --exit-with-session

# Run the test!
$(addprefix debian/stamp-,$(TESTS)):
	HOME="$(CURDIR)/$(MOZ_DISTDIR)" \
	$(WRAPPER) $(MAKE) -C $(CURDIR)/$(MOZ_OBJDIR) $(subst debian/stamp-,,$@) || true
	touch $@
