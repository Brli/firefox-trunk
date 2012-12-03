Cu.import("resource://gre/modules/AddonManager.jsm");

function run_test()
{
  do_check_true(!!_TEST_SELECTED_LOCALE);
  _XPCSHELL_PROCESS = "child-" + Cc["@mozilla.org/process/environment;1"].getService(Ci.nsIEnvironment).get("LC_ALL");

  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1");
  Services.prefs.setCharPref("extensions.minCompatibleAppVersion", "0.1");

  Cc["@mozilla.org/addons/integration;1"].getService(Ci.nsIObserver).observe(null, "addons-startup", null);

  do_test_pending();

  AddonManager.getInstallForFile(do_get_file("data/locale-matchOS-test-addon.xpi"),
                                 function(install) {
    install.addListener({
      onInstallEnded: function(install, addon) {
        do_execute_soon(function() {
          do_check_eq(Cc["@mozilla.org/chrome/chrome-registry;1"].getService(Ci.nsIXULChromeRegistry).getSelectedLocale("test"), _TEST_SELECTED_LOCALE);

          do_test_finished();
        });
      },

      onInstallFailed: function(install) {
        do_check_true(false);
        do_test_finished();
      }
    });
    install.install();
  });
}
