Cu.import("resource://gre/modules/AddonManager.jsm");

function run_test()
{
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
  do_test_pending();

  let istream = Services.io.newChannelFromURI(Services.io.newFileURI(do_get_file("data/locales.shipped"))).open();

  let line = { value: "" };
  let locales = {};
  while (istream.readLine(line)) {
    if (!line.value.match(/^\s*#.*/)) {
      locales[line.value.replace(/^([^:]*).*/, "$1")] = false;
    }
  }

  AddonManager.getAddonsByTypes(["locale"], function(addons) {
    let re = /langpack-([a-zA-Z\-]+)@firefox.mozilla.org/;

    addons.forEach(function(addon) {
      let m = addon.id.match(re);
      if (!m) {
        return;
      }

      if (m[1] in locales && !addon.appDisabled) {
        locales[m[1]] = true;
      }
    });

    Object.keys(locales).forEach(function(locale) {
      do_print("Checking if the addon manager found a language pack for " + locale);
      do_check_true(locales[locale]);
    });

    do_test_finished();
  });
}
