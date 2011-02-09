// Use LANG environment variable to choose locale
pref("intl.locale.matchOS", true);

// Disable default browser checking.
pref("browser.shell.checkDefaultBrowser", false);

// Prevent EULA dialog to popup on first run
pref("browser.EULA.override", true);

// identify default locale to use if no /usr/lib/firefox-addons/searchplugins/LOCALE
// exists for the current used LOCALE
pref("distribution.searchplugins.defaultLocale", "en-US");

// Enable the NetworkManager integration
pref("toolkit.networkmanager.disable", false);