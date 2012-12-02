function do_test(aLocale, aCallback)
{
  do_test_pending();
  do_print("Starting test for " + aLocale);
  do_run_test_in_subprocess_with_params("test_searchplugin_valid_real.js",
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
  let locale;
  if ((locale = aLocales.shift())) {
    do_execute_soon(function() {
      do_test(locale, function() {
        maybe_schedule_next_test(aLocales);
      });
    });
  }
}

function run_test()
{
  _XPCSHELL_PROCESS = "parent";

  let istream = Services.io.newChannelFromURI(Services.io.newFileURI(do_get_file("data/locales.shipped"))).open();

  let line = { value: "" };
  let tests = [];
  while (istream.readLine(line)) {
    if (!line.value.match(/^\s*#.*/)) {
      tests.push(line.value.replace(/^([^:]*).*/, "$1"));
    }
  }

  maybe_schedule_next_test(tests);
}
