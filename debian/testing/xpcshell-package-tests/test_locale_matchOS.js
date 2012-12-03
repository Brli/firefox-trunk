gLocales = {
  "en_US.UTF-8": "en-US",
  "fr_FR.UTF-8": "fr",
  "fr_BE.UTF-8": "fr",
  "pt_BR.UTF-8": "pt-BR",
  "pt_PT.UTF-8": "pt-BR",
  "de_DE.UTF-8": "en-US",
  "fr_BE.utf-8": "fr"
};

function do_test(aLocale, aCallback)
{
  do_test_pending();
  do_print("Starting test for " + aLocale);
  do_run_test_in_subprocess_with_params("test_locale_matchOS_real.js",
                                        { "_TEST_SELECTED_LOCALE": gLocales[aLocale] },
                                        { "LC_ALL": aLocale },
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

  try {
    do_check_true(Services.prefs.getBoolPref("intl.locale.matchOS"));
  } catch(e) {
    do_check_true(false);
  }

  maybe_schedule_next_test(["en_US.UTF-8", "fr_FR.UTF-8", "fr_BE.UTF-8",
                            "pt_BR.UTF-8", "pt_PT.UTF-8", "de_DE.UTF-8",
                            "fr_BE.utf-8"]);
}
