function do_test(aLocale, aCallback)
{
  do_test_pending();
  do_print("Starting test for " + aLocale);
  do_run_test_in_subprocess_with_params("test_ubuntu_searchplugins_real.js",
                                        { "_SEARCHPLUGIN_TEST_LOCALE": aLocale },
                                        function(aSuccess) {
    do_check_true(aSuccess);
    do_print("Finished test for " + aLocale);
    aCallback();
    do_test_finished();
  });
}

function maybe_schedule_next_test(aLocales)
{
  if (aLocales.hasMore()) {
    do_execute_soon(function() {
      do_test(aLocales.getNext(), function() {
        maybe_schedule_next_test(aLocales);
      });
    });
  }
}

function run_test()
{
  _XPCSHELL_PROCESS = "parent";

  let inifactory = Cc["@mozilla.org/xpcom/ini-parser-factory;1"].getService(Ci.nsIINIParserFactory);
  let parser = inifactory.createINIParser(do_get_file("searchplugins.list"));

  let locales = parser.getKeys("Searchplugins");
  maybe_schedule_next_test(locales);
}
