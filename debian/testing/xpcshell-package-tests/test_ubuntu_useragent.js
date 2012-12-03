function run_test()
{
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1");

  let ua = Services.io.newChannel("http://foo", null, null).QueryInterface(Ci.nsIHttpChannel).getRequestHeader("User-Agent");
  do_check_eq(ua, ua.match(/^Mozilla\/5\.0 \(X11; Ubuntu; Linux [^;]*; rv:[0-9\.]*\) Gecko\/20121202 Firefox\/[0-9\.]*/));
}
