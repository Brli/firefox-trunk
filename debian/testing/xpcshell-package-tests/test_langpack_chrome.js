Cu.import("resource://gre/modules/AddonManager.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");

function run_test()
{
  Services.prefs.setBoolPref("intl.locale.matchOS", false);

  let appIni = Services.dirsvc.get("CurProcD", Ci.nsIFile);
  appIni.append("application.ini");

  let parser = Components.manager.getClassObjectByContractID("@mozilla.org/xpcom/ini-parser-factory;1", Ci.nsIINIParserFactory).createINIParser(appIni);

  createAppInfo(parser.getString("App", "ID"), parser.getString("App", "Name"), parser.getString("App", "Version"), parser.getString("Gecko", "MaxVersion"));

  Cc["@mozilla.org/addons/integration;1"].getService(Ci.nsIObserver).observe(null, "addons-startup", null);

  // We've started the addon manager, but need to manually register addon chrome
  // for non-restartless addons. This is normally handled in
  // toolkit/xre/nsXREDirProvider.cpp, which is not available to xpcshell. When
  // we use restartless language packs, this can go away
  let extensions_ini = Services.dirsvc.get("ProfD", Ci.nsIFile);
  extensions_ini.append("extensions.ini");

  let re = /langpack-[a-zA-Z\-]+@firefox.mozilla.org.xpi/;
  parser = Components.manager.getClassObjectByContractID("@mozilla.org/xpcom/ini-parser-factory;1", Ci.nsIINIParserFactory).createINIParser(extensions_ini);
  let e = parser.getKeys("ExtensionDirs");
  while (e.hasMore()) {
    let k = e.getNext();
    let file = new FileUtils.File(parser.getString("ExtensionDirs", k));
    if (file.leafName.match(re)) {
      Components.manager.addBootstrappedManifestLocation(file);
    }
  }

  let istream = Services.io.newChannelFromURI(Services.io.newFileURI(do_get_file("data/locales.shipped"))).open();

  let line = { value: "" };
  let locales = [];
  while (istream.readLine(line)) {
    if (!line.value.match(/^\s*#.*/)) {
      locales.push(line.value.replace(/^([^:]*).*/, "$1"))
    }
  }

  // For most locales, "chrome://global/locale/intl.properties:general.useragent.locale"
  // is set to the locale. There are some exceptions though, listed here:
  const OVERRIDES = {
    "bn-BD": "en-US",
    "cs": "cs-CZ",
    "da": "da-DK",
    "el": "el-GR",
    "fa": "fa-IR",
    "fi": "fi-FI",
    "gl": "gl-ES",
    "he": "he-IL",
    "hi-IN": "en-US",
    "hu": "hu-HU",
    "is": "is-IS",
    "kn": "kn-IN",
    "ko": "ko-KR",
    "lv": "lv-LV",
    "mai": "en-US",
    "mk": "mk-MK",
    "mr": "mr-IN",
    "pa-IN": "pa",
    "pl": "pl-PL",
    "si": "si-LK",
    "te": "te-IN",
    "tr": "tr-TR"
  };

  locales.forEach(function(locale) {
    do_print("Testing locale " + locale);

    Services.strings.flushBundles();
    Services.prefs.setCharPref("general.useragent.locale", locale);

    if (locale in OVERRIDES) {
      locale = OVERRIDES[locale];
    }

    do_check_eq(Services.strings.createBundle("chrome://global/locale/intl.properties").GetStringFromName("general.useragent.locale"), locale);
  });
}
