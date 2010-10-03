#!/usr/bin/make -f

LOCALE		:= en_US.UTF-8
LOCDIR		= $(CURDIR)/$(MOZ_OBJDIR)/dist/.locales

TESTS	:= $(NULL)
ifeq (1,$(DEB_WANT_UNIT_TESTS))
	TESTS += check xpcshell-tests reftest crashtest
endif

debian/stamp-testsuite: $(TESTS)
	touch $@

# Required for js/src/trace-tests/sunspider/check-date-format-tofte.js
check: export TZ = :/usr/share/zoneinfo/posix/US/Pacific

$(LOCDIR)/%:
	mkdir -p $(LOCDIR)
	localedef -f $(shell echo $(notdir $@) | cut -d '.' -f 2) -i $(shell echo $(notdir $@) | cut -d '.' -f 1) $@

# Setup locales for tests which need it
xpcshell-tests reftest: $(LOCDIR)/$(LOCALE)
xpcshell-tests reftest: export LOCPATH=$(LOCDIR)
xpcshell-tests reftest: export LC_ALL=$(LOCALE)

# Disable tests that are known to fail
xpcshell-tests: xpcshell-tests-disable

# Tests that need a X server
reftest crashtest: WRAPPER = xvfb-run -s "-screen 0 1024x768x24"

# Run the test!
$(TESTS):
	HOME="$(CURDIR)/$(MOZ_OBJDIR)/dist" \
	$(WRAPPER) $(MAKE) -C $(CURDIR)/$(MOZ_OBJDIR) $@ || true

xpcshell-tests-disable:
	# Hangs without network access
	rm -f $(CURDIR)/$(MOZ_OBJDIR)/_tests/xpcshell/toolkit/components/places/tests/unit/test_404630.js

	# FIXME: IPC tests seem to hang in the buildd's
	rm -rf $(CURDIR)/$(MOZ_OBJDIR)/_tests/xpcshell/chrome/test/unit_ipc
	rm -rf $(CURDIR)/$(MOZ_OBJDIR)/_tests/xpcshell/ipc/testshell/tests
	rm -rf $(CURDIR)/$(MOZ_OBJDIR)/_tests/xpcshell/toolkit/components/contentprefs/tests/unit_ipc
	rm -rf $(CURDIR)/$(MOZ_OBJDIR)/_tests/xpcshell/ipc
	rm -rf $(CURDIR)/$(MOZ_OBJDIR)/_tests/xpcshell/netwerk/cookie/test/unit_ipc
	rm -rf $(CURDIR)/$(MOZ_OBJDIR)/_tests/xpcshell/netwerk/test/unit_ipc
	rm -rf $(CURDIR)/$(MOZ_OBJDIR)/_tests/xpcshell/modules/libpref/test/unit_ipc

	# Needs GConf to be running. I guess we need to start with dbus-launch to fix this
	rm -f $(CURDIR)/$(MOZ_OBJDIR)/_tests/xpcshell/browser/components/shell/test/unit/test_421977.js
	rm -f $(CURDIR)/$(MOZ_OBJDIR)/_tests/xpcshell/uriloader/exthandler/tests/unit/test_handlerService.js
