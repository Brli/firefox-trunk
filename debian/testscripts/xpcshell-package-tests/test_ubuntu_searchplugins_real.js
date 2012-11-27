function get_query_params(aURL)
{
  let params = {};
  aURL.query.split('&').forEach(function(query) {
    let parts = query.split('=');
    do_check_eq(parts.length, 2);
    params[parts[0]] = parts[1];
  });

  return params;
}

function test_baidu(aEngine)
{
  let url = aEngine.getSubmission("foo").uri.QueryInterface(Ci.nsIURL);

  // We ship the upstream plugin for this one
  if (url.host == "zhidao.baidu.com") {
    return false;
  }

  let params = get_query_params(url);

  let ubuntu = "tn" in params && params["tn"] == "ubuntuu_cb" && "cl" in params && params["cl"] == 3;

  // We only expect to see our Baidu searchplugin
  do_check_true(ubuntu);

  return ubuntu;
}

function test_duckduckgo(aEngine)
{
  let url = aEngine.getSubmission("foo").uri.QueryInterface(Ci.nsIURL);

  let params = get_query_params(url);
  let ubuntu = "t" in params && params["t"] == "canonical";

  // We only expect to see our DDG searchplugin
  do_check_true(ubuntu);

  return ubuntu;
}

function test_amazon(aEngine)
{
  let url = aEngine.getSubmission("foo").uri.QueryInterface(Ci.nsIURL);

  let params = get_query_params(url);
  let ubuntu = "tag" in params && params["tag"] == "wwwcanoniccom-20";

  // We only expect to see our Amazon searchplugin
  do_check_true(ubuntu);

  return ubuntu;
}

function test_google(aEngine)
{
  let wanted = {
    "gl": {
      "en-GB": "uk",
      "en-ZA": "za"
    },
    "hl": {
      "ku": "en",
      "ja": "ja"
    }    
  };

  function check_extra_params() {
    for (let param in wanted) {
      do_check_eq(params[param], wanted[param][_SEARCHPLUGIN_TEST_LOCALE]);
    }
  }

  do_check_eq(Services.io.newURI(aEngine.searchForm, null, null).scheme, "https");

  let url = aEngine.getSubmission("foo").uri.QueryInterface(Ci.nsIURL);
  // Verify we are using a secure URL
  do_check_eq(url.scheme, "https");

  let params = get_query_params(url);

  let ubuntu = "client" in params && params["client"] == "ubuntu" && "channel" in params && params["channel"] == "fs";

  // We only expect to see our Google searchplugin
  do_check_true(ubuntu);

  check_extra_params();

  url = aEngine.getSubmission("foo", "application/x-suggestions+json").uri.QueryInterface(Ci.nsIURL);
  // Verify we are using a secure URL for suggestions
  do_check_eq(url.scheme, "https");

  params = get_query_params(url);

  // "client=ubuntu" fails for suggestions
  do_check_eq(params["client"], "firefox");

  check_extra_params();

  return ubuntu;
}

function run_test()
{
  do_check_true(!!_SEARCHPLUGIN_TEST_LOCALE);
  _XPCSHELL_PROCESS = "child-" + _SEARCHPLUGIN_TEST_LOCALE;

  //Services.prefs.setBoolPref("browser.search.log", true);
  Services.prefs.setCharPref("general.useragent.locale", _SEARCHPLUGIN_TEST_LOCALE);
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1");

  let found_Google = false;
  let found_Amazon = false;
  let found_DDG = false;
  let found_Baidu = false;

  let want_Baidu = false;
  let want_Amazon = false;

  if (_SEARCHPLUGIN_TEST_LOCALE == "zh-CN") {
    want_Baidu = true;
  }

  if (["en-US", "af", "ar", "bg", "bn-IN", "br", "bs", "cy", "da", "de", "el",
       "en-GB", "en-ZA", "eo", "es-AR", "eu", "fa", "fr", "ga-IE", "gd", "gl",
       "hr", "hy-AM", "is", "it", "ja", "kn", "ku", "lg", "lt", "mk", "mr",
       "nb-NO", "nn-NO", "nso", "or", "pt-PT", "ro", "si", "sq", "sr", "te",
       "th", "tr", "zh-CN", "zu"].indexOf(_SEARCHPLUGIN_TEST_LOCALE) != -1) {
    want_Amazon = true;
  }

  do_test_pending();

  Services.search.init({
    onInitComplete: function(aStatus) {
      do_check_true(Components.isSuccessCode(aStatus));

      Services.search.getEngines().forEach(function(engine) {
        let host = engine.getSubmission("foo").uri.host;
        if (host.match(/\.google\./)) {
          let is_ours = test_google(engine);
          do_check_true(!(is_ours && found_Google));
          found_Google = is_ours || found_Google;
        } else if (host.match(/\.amazon\./)) {
          let is_ours = test_amazon(engine);
          do_check_true(!(is_ours && found_Amazon));
          found_Amazon = is_ours || found_Amazon;
        } else if (host.match(/duckduckgo\./)) {
          let is_ours = test_duckduckgo(engine);
          do_check_true(!(is_ours && found_DDG));
          found_DDG = is_ours || found_DDG;
        } else if (host.match(/baidu\./)) {
          let is_ours = test_baidu(engine);
          do_check_true(!(is_ours && found_Baidu));
          found_Baidu = is_ours || found_Baidu;
        }
      });

      do_check_true(found_Google);
      do_check_true(!((found_Amazon && !want_Amazon) || (!found_Amazon && want_Amazon)));
      do_check_true(found_DDG);
      do_check_true(!((found_Baidu && !want_Baidu) || (!found_Baidu && want_Baidu)));

      do_test_finished();
    }
  });
}
