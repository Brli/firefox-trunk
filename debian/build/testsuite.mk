#!/usr/bin/make -f

ifeq (1,$(MOZ_WANT_UNIT_TESTS))
ifeq (,$(MOZ_TESTS))
TESTS = check
else
TESTS = $(MOZ_TESTS)
endif
endif

run-tests: $(addprefix debian/stamp-,$(TESTS))

$(addprefix debian/stamp-,$(TESTS)): debian/stamp-makefile-build

# Run the test!
$(addprefix debian/stamp-,$(TESTS)):
	HOME="$(CURDIR)/$(MOZ_DISTDIR)" \
	$(MAKE) -C $(CURDIR)/$(MOZ_OBJDIR) $(subst debian/stamp-,,$@) || true
	touch $@
