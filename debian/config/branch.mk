CHANNEL			= nightly
MOZ_WANT_UNIT_TESTS	= 1
NO_AUTO_REFRESH_LOCALES	= 1

ifeq (1,$(shell test "$(DISTRIB_VERSION_MAJOR)$(DISTRIB_VERSION_MINOR)" -ge "1104" && echo "1"))
# Enable crashreporter on nightly builds newer than Maverick
MOZ_ENABLE_BREAKPAD = 1
endif
