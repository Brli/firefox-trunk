#!/usr/bin/make -f

ifeq (1,$(MOZ_WANT_UNIT_TESTS))
ifeq (,$(MOZ_TESTS))
TESTS = check mochitest
else
TESTS = $(MOZ_TESTS)
endif
endif

run-tests: $(addprefix debian/stamp-,$(TESTS))

$(addprefix debian/stamp-,$(TESTS)): debian/stamp-makefile-build

# Tests that need a X server
$(addprefix debian/stamp-,mochitest): WRAPPER = xvfb-run -a -s "-screen 0 1024x768x24" dbus-launch --exit-with-session

# Run the test!
$(addprefix debian/stamp-,$(TESTS)):
	HOME="$(CURDIR)/$(MOZ_DISTDIR)" \
	$(WRAPPER) $(MAKE) -C $(CURDIR)/$(MOZ_OBJDIR) $(subst debian/stamp-,,$@) || true
	touch $@
