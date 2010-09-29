#!/usr/bin/make -f

MOZ_BUILD_ROOT	:= $(shell echo `pwd`)
TESTS 		:= check xpcshell-tests reftest crashtest
LOCALE		:= en_US.UTF-8
LOCDIR		:= $(MOZ_BUILD_ROOT)/dist/.locales

# Run all testsuite targets
test: $(TESTS)

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
	HOME="$(MOZ_BUILD_ROOT)/dist" \
	$(WRAPPER) $(MAKE) -C $(MOZ_BUILD_ROOT) $@ || true

xpcshell-tests-disable:
	# Hangs without network access
	rm -f $(MOZ_BUILD_ROOT)/_tests/xpcshell/toolkit/components/places/tests/unit/test_404630.js

	# FIXME: IPC tests seem to hang in the buildd's
	rm -rf $(MOZ_BUILD_ROOT)/_tests/xpcshell/chrome/test/unit_ipc
	rm -rf $(MOZ_BUILD_ROOT)/_tests/xpcshell/ipc/testshell/tests
	rm -rf $(MOZ_BUILD_ROOT)/_tests/xpcshell/toolkit/components/contentprefs/tests/unit_ipc
	rm -rf $(MOZ_BUILD_ROOT)/_tests/xpcshell/ipc
	rm -rf $(MOZ_BUILD_ROOT)/_tests/xpcshell/netwerk/cookie/test/unit_ipc
	rm -rf $(MOZ_BUILD_ROOT)/_tests/xpcshell/netwerk/test/unit_ipc
	rm -rf $(MOZ_BUILD_ROOT)/_tests/xpcshell/modules/libpref/test/unit_ipc

	# Needs GConf to be running. I guess we need to start with dbus-launch to fix this
	rm -f $(MOZ_BUILD_ROOT)/_tests/xpcshell/browser/components/shell/test/unit/test_421977.js
	rm -f $(MOZ_BUILD_ROOT)/_tests/xpcshell/uriloader/exthandler/tests/unit/test_handlerService.js
