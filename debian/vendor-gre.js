// Use LANG environment variable to choose locale
pref("intl.locale.matchOS", true);

// Enable Network Manager integration
pref("network.manage-offline-status", true);

// Load system dictionaries. Note that this doesn't work in distribution.ini
// because that is applied after addons-startup, when the dictionaries are
// loaded
pref("spellchecker.dictionary_path", "/usr/share/hunspell");

// Use the system locale. Note that this doesn't work correctly in
// distribution.ini as this pref needs to be initialized before
// distribution.ini prefs are applied, in order for locale-specific prefs
// to work
pref("intl.locale.requested", "");
