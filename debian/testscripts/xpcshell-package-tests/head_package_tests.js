const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu } = Components;

const NS_GRE_DIR                        = "GreD";
const XRE_EXECUTABLE_FILE               = "XREExeF";
const NS_XPCOM_INIT_CURRENT_PROCESS_DIR = "MozBinD";

Cu.import("resource://gre/modules/Services.jsm");

do_get_profile();

var provider = {
  getFile: function() {
    throw Cr.NS_ERROR_FAILURE;
  },

  getFiles: function(prop) {
    if (prop == "XREExtDL") {
      return {
        getNext: function() {
          throw Cr.NS_ERROR_FAILURE;
        },
        hasMoreElements: function() {
          return false;
        },
        QueryInterface: function(iid) {
          if (iid.equals(Ci.nsISimpleEnumerator) ||
              iid.equals(Ci.nsISupports)) {
            return this;
          }
          throw Cr.NS_ERROR_NO_INTERFACE;
        }
      };
    }
    throw Cr.NS_ERROR_FAILURE;
  },

  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIDirectoryServiceProvider2) ||
        iid.equals(Ci.nsIDirectoryServiceProvider) ||
        iid.equals(Ci.nsISupports)) {
      return this;
    }
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
};

Services.dirsvc.registerProvider(provider);

function createAppInfo(id, name, version, platformVersion)
{
  gXULAppInfo = {
    vendor: "Mozilla",
    name: name,
    ID: id,
    version: version,
    appBuildID: "2007010101",
    platformVersion: platformVersion,
    platformBuildID: "2007010101",
    inSafeMode: false,
    logConsoleErrors: true,
    OS: "XPCShell",
    XPCOMABI: "noarch-spidermonkey",
    invalidateCachesOnRestart: function invalidateCachesOnRestart() {},

    QueryInterface: function QueryInterface(iid) {
      if (iid.equals(Ci.nsIXULAppInfo)
       || iid.equals(Ci.nsIXULRuntime)
       || iid.equals(Ci.nsISupports))
        return this;

      throw Cr.NS_ERROR_NO_INTERFACE;
    }
  };

  var XULAppInfoFactory = {
    createInstance: function (outer, iid) {
      if (outer != null)
        throw Cr.NS_ERROR_NO_AGGREGATION;
      return gXULAppInfo.QueryInterface(iid);
    }
  };

  var registrar = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);
  registrar.registerFactory(Components.ID("{ee408b20-f0c6-4474-bdfe-f8a1f377ca3d}"),
                            "XULAppInfo",
                            "@mozilla.org/xre/app-info;1", XULAppInfoFactory);
}

function do_run_test_in_subprocess_with_params(aTestFile, aParams, aCallback)
{
  let proc = Cc["@mozilla.org/process/util;1"].createInstance(Ci.nsIProcess);
  proc.init(Services.dirsvc.get("XREExeF", Ci.nsIFile));

  function addQuotes(str) {
    return "\"" + str + "\"";
  }

  let head_files = _HEAD_FILES.map(addQuotes);
  let tail_files = _TAIL_FILES.map(addQuotes);

  let args = [ "-g", Services.dirsvc.get(NS_GRE_DIR, Ci.nsIFile).path,
               "-a", Services.dirsvc.get(NS_XPCOM_INIT_CURRENT_PROCESS_DIR, Ci.nsIFile).path,
               "-m", "-n", "-s",
               "-e", "const _HEAD_JS_PATH = \"" + _HEAD_JS_PATH + "\";",
               "-e", "const _TESTING_MODULES_DIR = \"" + _TESTING_MODULES_DIR + "\";",
               "-f", _HEAD_JS_PATH,
               "-e", "const _HEAD_FILES = [" + head_files.join() + "];",
               "-e", "const _TAIL_FILES = [" + tail_files.join() + "];",
               "-e", "const _TEST_FILE = [\"" + do_get_file(aTestFile).path + "\"];" ];

  if (aParams) {
    for (let param in aParams) {
      args.push("-e", "const " + param + " = \"" + aParams[param] + "\";");
    }
  }

  args.push("-e", "_execute_test(); quit(_passed ? 0 : -1);");

  do_print("Running " + aTestFile + " in subprocess");

  proc.runAsync(args, args.length, {
                observe: function(subject, topic, data) {
    if (aCallback) {
      aCallback(topic == "process-finished" && proc.exitValue == 0);
    }
  }}, false);
}
