function run_test()
{
  do_check_true(!!_SEARCHPLUGIN_TEST_LOCALE);
  _XPCSHELL_PROCESS = "child-" + _SEARCHPLUGIN_TEST_LOCALE;

  //Services.prefs.setBoolPref("browser.search.log", true);
  Services.prefs.setCharPref("general.useragent.locale", _SEARCHPLUGIN_TEST_LOCALE);
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1");

  let inifactory = Cc["@mozilla.org/xpcom/ini-parser-factory;1"].getService(Ci.nsIINIParserFactory);
  let parser = inifactory.createINIParser(do_get_file("searchplugins.list"));

  let searchplugins;
  try {
    searchplugins = parser.getString("Searchplugins", _SEARCHPLUGIN_TEST_LOCALE).split(",");
  } catch(e) {}

  do_check_true(searchplugins != undefined);

  parser = inifactory.createINIParser(do_get_file("searchplugin-additions.list"));

  try {
    parser.getString("Additions", _SEARCHPLUGIN_TEST_LOCALE).split(",").forEach(function(name) {
      searchplugins.push(name);
    });
  } catch(e) {}

  do_test_pending();

  Services.search.init({
    onInitComplete: function(aStatus) {
      do_check_true(Components.isSuccessCode(aStatus));

      // Check that the search service loaded the expected number of search
      // plugins for this locale
      do_check_eq(Services.search.getEngines().length, searchplugins.length);

      do_test_finished();
    }
  });
}
