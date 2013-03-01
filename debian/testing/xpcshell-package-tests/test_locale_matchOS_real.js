Cu.import("resource://gre/modules/AddonManager.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");

function run_test()
{
  do_check_true(!!_TEST_SELECTED_LOCALE);
  _XPCSHELL_PROCESS = "child-" + Cc["@mozilla.org/process/environment;1"].getService(Ci.nsIEnvironment).get("LC_ALL");

  let appIni = Services.dirsvc.get("CurProcD", Ci.nsIFile).parent;
  appIni.append("application.ini");
  let appIniParser = Components.manager.getClassObjectByContractID("@mozilla.org/xpcom/ini-parser-factory;1",
                                                                   Ci.nsIINIParserFactory).createINIParser(appIni);

  let platformIni = Services.dirsvc.get("GreD", Ci.nsIFile);
  platformIni.append("platform.ini");
  let platformIniParser = Components.manager.getClassObjectByContractID("@mozilla.org/xpcom/ini-parser-factory;1",
                                                                        Ci.nsIINIParserFactory).createINIParser(platformIni);

  createAppInfo(appIniParser.getString("App", "ID"), appIniParser.getString("App", "Name"),
                appIniParser.getString("App", "Version"), platformIniParser.getString("Build", "Milestone"));

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

  let re = new RegExp(_TEST_SELECTED_LOCALE);
  let selected = Cc["@mozilla.org/chrome/chrome-registry;1"].getService(Ci.nsIXULChromeRegistry).getSelectedLocale("global");
  do_check_eq(selected.match(re), selected);
}
