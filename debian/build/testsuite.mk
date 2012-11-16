#!/usr/bin/make -f

MOZ_TEST_LOCALE ?= en_US.UTF-8

MOZ_TESTS ?= check
MOZ_TEST_FAILURES_FATAL ?= 1

MOZ_TEST_X_WRAPPER ?= xvfb-run -a -s "-screen 0 1024x768x24 -extension=MIT-SCREEN-SAVER" dbus-launch --exit-with-session
MOZ_TESTS_NEED_X ?= xpcshell-tests jstestbrowser reftest crashtest mochitest

MOZ_TESTS_TZ_ENV ?= TZ=:/usr/share/zoneinfo/posix/US/Pacific
MOZ_TESTS_NEED_TZ ?= check jstestbrowser

MOZ_TESTS_NEED_LOCALE ?= xpcshell-tests jstestbrowser reftest

TEST_LOCALES = $(CURDIR)/$(MOZ_OBJDIR)/_ubuntu_build_test_tmp/locales
TEST_HOME = $(CURDIR)/$(MOZ_OBJDIR)/_ubuntu_build_test_tmp/home

GET_WRAPPER = $(if $(filter $(1),$(MOZ_TESTS_NEED_X)),$(MOZ_TEST_X_WRAPPER))
GET_TZ = $(if $(filter $(1),$(MOZ_TESTS_NEED_TZ)),$(MOZ_TESTS_TZ_ENV))

DOIF_NEEDS_LOCALE = $(if $(filter $(1),$(MOZ_TESTS_NEED_LOCALE)),$(call $(2)))
MAKE_LOCALE = $(TEST_LOCALES)/$(MOZ_TEST_LOCALE)
GET_LOCALE_ENV = LOCPATH=$(TEST_LOCALES) LC_ALL=$(MOZ_TEST_LOCALE)

ifneq (1,$(MOZ_TEST_FAILURES_FATAL))
CMD_APPEND = || true
endif

ifneq (1,$(MOZ_WANT_UNIT_TESTS))
MOZ_TESTS =
endif

$(TEST_LOCALES) $(TEST_HOME):: %:
	mkdir -p $@

$(TEST_LOCALES)/$(MOZ_TEST_LOCALE): $(TEST_LOCALES)
	localedef -f $(shell echo $(notdir $@) | cut -d '.' -f 2) -i $(shell echo $(notdir $@) | cut -d '.' -f 1) $@

run-tests: $(MOZ_TESTS)

$(MOZ_TESTS):: %: debian/stamp-test-%

$(patsubst %,debian/stamp-test-%,$(MOZ_TESTS)):: TZ=$(call GET_TZ,$*)
$(patsubst %,debian/stamp-test-%,$(MOZ_TESTS)):: WRAPPER=$(call GET_WRAPPER,$*)
$(patsubst %,debian/stamp-test-%,$(MOZ_TESTS)):: $(call DOIF_NEEDS_LOCALE,$*,MAKE_LOCALE)
$(patsubst %,debian/stamp-test-%,$(MOZ_TESTS)):: LOCALE_ENV=$(call DOIF_NEEDS_LOCALE,$*,GET_LOCALE_ENV)
$(patsubst %,debian/stamp-test-%,$(MOZ_TESTS)):: $(TEST_HOME)
$(patsubst %,debian/stamp-test-%,$(MOZ_TESTS)):: TEST_CMD=HOME=$(TEST_HOME) $(LOCALE_ENV) $(TZ) $(WRAPPER) $(MAKE) -C $(CURDIR)/$(MOZ_OBJDIR) $*
$(patsubst %,debian/stamp-test-%,$(MOZ_TESTS)):: debian/stamp-test-%: debian/stamp-makefile-build
	@echo "\nRunning $(TEST_CMD)\n"
	@$(TEST_CMD) $(CMD_APPEND)
	@touch $@

.PHONY: run-tests $(MOZ_TESTS)
