#!/usr/bin/python

from __future__ import print_function
from fnmatch import fnmatch
from glob import glob
from optparse import OptionParser
import json
import os.path
import shutil
import sys

def find_locale_match(locale, locales):
  return any(fnmatch(locale, i) for i in locales)

def install_one_override(config, options, upstream_config):
  print("Handling override for '%s', %s locale" % (config["name"], options.locale))

  if ("exclude_locales" in config and
      find_locale_match(options.locale, config["exclude_locales"])):
    print("No override for this locale (exclude_locales)")
    return

  engines = config["engines"]
  include = find_locale_match(options.locale, config["include_locales"])

  upstream_l10n_config = {}
  if options.locale in upstream_config["locales"]:
    upstream_l10n_config = upstream_config["locales"][options.locale]

  upstream_visible_engines = []
  for i in [ upstream_l10n_config, upstream_config ]:
    if "default" in i and "visibleDefaultEngines" in i["default"]:
      upstream_visible_engines = i["default"]["visibleDefaultEngines"]
      break

  found = []
  for i in engines:
    if any(j == i for j in upstream_visible_engines):
      found.append(i)

  if len(found) == 0:
    if include:
      print("No search plugin to override for '%s', %s locale" % (config["name"], options.locale), file=sys.stderr)
      sys.exit(1)
    print("No override for this locale (include_locales)")
    return

  if not include:
    print("Found search plugin to override for '%s', %s locale, even though it's not in "
          "include_locales. Please check if it should be included" % (config["name"], options.locale),
          file=sys.stderr)
    sys.exit(1)

  if len(found) > 1:
    print("More than one search plugin found for '%s', %s locale" % (config["name"], options.locale), file=sys.stderr)
    sys.exit(1)

  try:
    os.makedirs(options.destdir)
  except:
    pass

  filename = found[0] + ".xml"
  s = os.path.join(options.srcdir, filename)
  d = os.path.join(options.destdir, filename)
  print("Installing %s in to %s" % (s, d))
  shutil.copy(s, d)

def install_overrides(config, options, upstream_config):
  for i in config: install_one_override(i, options, upstream_config)

def install_one_addition(config, options, upstream_config):
  print("Handling addition for '%s', %s locale" % (config["name"], options.locale))

  if ("exclude_locales" in config and
      find_locale_match(options.locale, config["exclude_locales"])):
    print("No addition for this locale (exclude_locales)")
    return

  if (not find_locale_match(options.locale, config["include_locales"])):
    print("No addition for this locale (include_locales)")
    return

  upstream_l10n_config = {}
  if options.locale in upstream_config["locales"]:
    upstream_l10n_config = upstream_config["locales"][options.locale]

  upstream_visible_engines = []
  for i in [ upstream_l10n_config, upstream_config ]:
    if "default" in i and "visibleDefaultEngines" in i["default"]:
      upstream_visible_engines = i["default"]["visibleDefaultEngines"]

  if any(i == config["engine"] for i in upstream_visible_engines):
    print("There is already a searchplugin with this id for '%s', %s locale" %
          (config["name"], options.locale), file=sys.stderr)
    sys.exit(1)

  filename = config["engine"] + ".xml"
  s = os.path.join(options.srcdir, filename)
  d = os.path.join(options.destdir, filename)

  print("Installing %s in to %s" % (s, d))
  shutil.copy(s, d)

def install_additions(config, options, upstream_config):
  for i in config: install_one_addition(i, options, upstream_config)

def main(argv):
  parser = OptionParser("usage: %prog [options]")
  parser.add_option("-c", "--config", dest="config", help="Location of the config file")
  parser.add_option("-l", "--locale", dest="locale", help="The locale to install")
  parser.add_option("-u", "--stagedir", dest="stagedir")
  parser.add_option("-d", "--destdir", dest="destdir")
  parser.add_option("-s", "--srcdir", dest="srcdir")

  (options, args) = parser.parse_args(args=argv)

  if (any(getattr(options, o) == None for o in ["config", "locale", "stagedir", "destdir", "srcdir"])):
    print("Missing option", file=sys.stderr)
    sys.exit(1)

  config = {}
  with open(options.config, "r") as fd:
    config = json.load(fd)

  upstream_config = {}
  with open(os.path.join(options.stagedir, "list.json"), "r") as fd:
    upstream_config = json.load(fd)

  if "overrides" in config:
    install_overrides(config["overrides"], options, upstream_config)
  if "additions" in config:
    install_additions(config["additions"], options, upstream_config)

if __name__ == "__main__":
  main(sys.argv[1:])
