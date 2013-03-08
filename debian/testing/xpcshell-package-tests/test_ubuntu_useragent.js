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

  createAppInfo(appIniParser.getString("App", "ID"), "Firefox",
                appIniParser.getString("App", "Version"),
                platformIniParser.getString("Build", "Milestone"));

  let ua = Services.io.newChannel("http://foo", null, null).QueryInterface(Ci.nsIHttpChannel).getRequestHeader("User-Agent");
  if (Services.prefs.getCharPref("app.update.channel") == "nightly") {
    var buildid = appIniParser.getString("App", "BuildID").slice(0, 8);
  } else {
    var buildid = "20100101";
  }
  let re = new RegExp("^Mozilla/5\\.0 \\(X11; Ubuntu; Linux [^;]*; rv:[0-9\\.]*\\) Gecko/" + buildid + " Firefox/[0-9\\.]*");
  do_check_eq(ua, ua.match(re));
}
