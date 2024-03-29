#!/usr/bin/python3

from datetime import datetime
from optparse import OptionParser
import os
import os.path
import re
import shutil
import subprocess
import sys
import time
import urllib.request
import xml.dom.minidom
import json
import tempfile
import select

def CheckCall(args, cwd=None, quiet=False, env=None):
  with open(os.devnull, "w") as devnull:
    p = subprocess.Popen(args, cwd=cwd, env=env,
                         stdout = devnull if quiet == True else None,
                         stderr = devnull if quiet == True else None)
    r = p.wait()
    if r != 0: raise subprocess.CalledProcessError(r, args)

def CheckOutput(args, cwd=None):
  p = subprocess.Popen(args, cwd=cwd, stdout=subprocess.PIPE, universal_newlines=True)
  r = p.wait()
  if r != 0: raise subprocess.CalledProcessError(r, args)
  return p.stdout.read()

def check_external_dependencies():
  for tool in ['hg', 'cargo', 'tar']:
    try:
      CheckCall([tool, '--version'], quiet=True)
    except OSError:
      print('Error: missing external tool: %s' % tool)
      sys.exit(1)

def ensure_cache(repo, cache):
  dest = os.path.join(cache, os.path.basename(repo))
  if os.path.isdir(dest):

    try:
      CheckCall(['hg', 'summary'], cwd=dest, quiet=True)
      print('Cache location %s exists, using it' % dest)
      CheckCall(['hg', 'pull', repo], cwd=dest)
      CheckCall(['hg', 'update'], cwd=dest)
      return
    except:
      pass

  if not os.path.isdir(cache):
    os.makedirs(cache)

  print('Creating cache location %s' % dest)
  CheckCall(['hg', 'clone', repo, dest])

def do_checkout(source, dest, rev=None):
  dest = os.path.abspath(dest)
  dest_parent = os.path.dirname(dest)
  if dest_parent != '' and not os.path.isdir(dest_parent):
    os.makedirs(dest_parent)

  CheckCall(['hg', 'clone', source, dest])

  args = ['hg', 'update']
  if rev != None:
    args.append('-r')
    args.append(rev)
  CheckCall(args, cwd=dest)

def checkout_source(repo, cache, dest, rev=None):
  print('*** Checking out source from %s%s ***' % (repo, ' using cache from %s' % cache if cache != None else ''))
  local_source = None
  if cache != None:
    ensure_cache(repo, cache)
    local_source = os.path.join(cache, os.path.basename(repo))
  source = repo if local_source == None else local_source
  do_checkout(source, dest, rev=rev)

def get_setting(settings, name, default=None):
  return settings[name] if name in settings else default

class ScopedTmpdir:
  def __enter__(self):
    self._tmpdir = tempfile.mkdtemp()
    return self._tmpdir

  def __exit__(self, type, value, traceback):
    shutil.rmtree(self._tmpdir)

class ScopedWorkingDirectory:
  def __init__(self, dir):
    self._wd = os.path.abspath(dir)

  def __enter__(self):
    self._saved_wd = os.getcwd()
    if not os.path.isdir(self._wd):
      os.makedirs(self._wd)

    os.chdir(self._wd)

  def __exit__(self, type, value, traceback):
    os.chdir(self._saved_wd)

class ScopedRename:
  def __init__(self, src, dest):
    self._src = src
    self._dest = dest

  def __enter__(self):
    os.rename(self._src, self._dest)

  def __exit__(self, type, value, traceback):
    os.rename(self._dest, self._src)

class TarballCreator(OptionParser):
  def __init__(self):
    OptionParser.__init__(self, 'usage: %prog [options]')

    self.add_option('-r', '--repo', dest='repo', help='The remote repository from which to pull the main source')
    self.add_option('-c', '--cache', dest='cache', help='A local cache of the remote repositories')
    self.add_option('-l', '--l10n-base-repo', dest='l10nbase', help='The base directory of the remote repositories to pull l10n data from')
    self.add_option('-v', '--version', dest='version', help='Release version to base the checkout on')
    self.add_option('--build', dest='build', help='Release build to base the checkout on (must be used with --version)')
    self.add_option('-n', '--name', dest='name', help='The package name')
    self.add_option('-b', '--basename', dest='basename', help='The package basename')
    self.add_option('-a', '--application', dest='application', help='The application to build')

  def vendor_cbindgen(self, dest):
    # cbindgen is not available in Debian/Ubuntu yet (https://bugs.debian.org/908312)
    print('*** Vendoring cbindgen and its dependencies ***')
    with ScopedTmpdir() as tmpdir:
      with ScopedWorkingDirectory(tmpdir):
        CheckCall(['cargo', 'new', 'vendored-cbindgen', '--vcs', 'none'])
        with ScopedWorkingDirectory('vendored-cbindgen'):
          with open('Cargo.toml', 'a+') as fd:
            fd.write('cbindgen = "=0.24.3"')
          CheckCall(['cargo', 'vendor'])
          with ScopedWorkingDirectory('vendor/cbindgen'):
            os.makedirs('.cargo')
            with open('.cargo/config', 'w') as fd:
              fd.write(CheckOutput(['cargo', 'vendor']))
            with open('Cargo.toml', 'a+') as fd:
              fd.write('\n[workspace]')
        shutil.copytree('vendored-cbindgen/vendor/cbindgen', dest)

  def vendor_dump_syms(self, dest):
    # dump_syms is not available in Debian/Ubuntu yet
    print('*** Vendoring dump_syms and its dependencies ***')
    with ScopedTmpdir() as tmpdir:
      with ScopedWorkingDirectory(tmpdir):
        CheckCall(['cargo', 'new', 'vendored-dump_syms', '--vcs', 'none'])
        with ScopedWorkingDirectory('vendored-dump_syms'):
          with open('Cargo.toml', 'a+') as fd:
            fd.write('dump_syms = "=2.2.0"')
          CheckCall(['cargo', 'vendor'])
          with ScopedWorkingDirectory('vendor/dump_syms'):
            os.makedirs('.cargo')
            with open('.cargo/config', 'w') as fd:
              fd.write(CheckOutput(['cargo', 'vendor']))
            with open('Cargo.toml', 'a+') as fd:
              fd.write('\n[workspace]')
        shutil.copytree('vendored-dump_syms/vendor/dump_syms', dest)

  def run(self):
    (options, args) = self.parse_args()

    if options.repo == None:
      self.error('Must specify a remote repository')

    if options.name == None:
      self.error('Must specify a package name')

    if options.application == None:
      self.error('Must specify an application')

    if options.build != None and options.version == None:
      self.error('--build must be used with --version')

    if options.cache != None and not os.path.isabs(options.cache):
      options.cache = os.path.join(os.getcwd(), options.cache)

    settings = None
    with open('debian/config/tarball.conf', 'r') as fd:
      settings = json.load(fd)

    repo = options.repo
    cache = options.cache
    version = options.version
    build = options.build
    application = options.application
    l10nbase = options.l10nbase
    name = options.name
    basename = options.basename
    if basename == None:
      basename = name

    main_rev = None
    buildid = None

    if version != None:
      if build == None:
        build = 1
      print('*** Determing revisions to use for checkouts ***')
      main_info_url = ('https://ftp.mozilla.org/pub/%s/candidates/%s-candidates/build%s/linux-x86_64/en-US/%s-%s.json'
                       % (basename, version, build, basename, version))
      u = urllib.request.urlopen(main_info_url)
      build_refs = json.load(u)
      main_rev = build_refs["moz_source_stamp"]
      if main_rev == None:
        print('Failed to determine revision for main checkout', file=sys.stderr)
        sys.exit(1)
      print('Revision to be used for main checkout: %s' % main_rev)
      buildid = build_refs["buildid"]
      if buildid == None:
        print('Invalid build ID', file=sys.stderr)
        sys.exit(1)
    else:
      buildid = datetime.now().strftime('%Y%m%d%H%M%S')
    print('Build ID: %s' % buildid)

    with ScopedTmpdir() as tmpdir:
      print('*** Using temporary directory %s ***' % tmpdir)
      orig_cwd = os.getcwd()
      with ScopedWorkingDirectory(os.path.join(tmpdir, name)):

        checkout_source(repo, cache, '', rev=main_rev)

        with open('SOURCE_CHANGESET', 'w') as fd:
          rev = CheckOutput(['hg', 'parent', '--template={node}'])
          fd.write(rev)

        self.vendor_cbindgen(os.path.join(os.getcwd(), 'third_party/cbindgen'))
        self.vendor_dump_syms(os.path.join(os.getcwd(), 'third_party/dump_syms'))

        l10ndir = 'l10n'
        if not os.path.isdir(l10ndir):
          os.makedirs(l10ndir)

        if l10nbase != None:
          got_locales = set()
          shipped_locales = os.path.join(application, 'locales/shipped-locales')
          blacklist_file = get_setting(settings, 'l10n-blacklist')

          with open(os.path.join(os.path.abspath(''), 'browser/locales/l10n-changesets.json'), 'r') as fd:
            locale_revs = json.load(fd)
            with open(shipped_locales, 'r') as fd:
              for line in fd:
                locale = line.split(' ')[0].strip()
                if locale.startswith('#') or locale in got_locales or locale == 'en-US':
                  continue

                try:
                  rev = None
                  if main_rev != None:
                    rev = locale_revs[locale]["revision"]
                    if rev == None:
                      print("Rev for locale '%s' is not present in l10n-changesets.json" % locale)
                      sys.exit(1)
                  checkout_source(os.path.join(l10nbase, locale), os.path.join(cache, 'l10n') if cache != None else None, 'l10n/' + locale, rev=rev)
                  
                  for line in CheckOutput(['hg', 'tip'], cwd='l10n/' + locale).split('\n'):
                    if line.startswith('changeset:'):
                      break

                  got_locales.add(locale)

                except Exception as e:
                  # checkout_locale will throw if the specified revision isn't found
                  # In this case, omit it from the tarball
                  print('Failed to checkout %s: %s' % (locale, e), file=sys.stderr)
                  localedir = os.path.join(l10ndir, locale)
                  if os.path.exists(localedir):
                    shutil.rmtree(localedir)

          # When we also use translations from Launchpad, there will be a file
          # containing the additional locales we want to ship (locales.extra??)
          print('*** Checking that required locales are present ***')

          blacklist = set()
          if blacklist_file:
            with open(os.path.join(orig_cwd, blacklist_file), 'r') as fd:
              for line in fd:
                locale = re.sub(r'([^#]*)#?.*', r'\1', line).strip()
                if locale == '':
                  continue

                blacklist.add(locale)

          with open(shipped_locales, 'r') as fd:
            for line in fd:
              line = line.strip()
              if line.startswith('#'):
                continue

              if line == 'en-US':
                continue

              locale = line.split(' ')[0].strip()
              platforms = line.split(' ')[1:]

              if locale in blacklist:
                continue

              if len(platforms) > 0:
                for_linux = False
                for platform in platforms:
                  if platform == 'linux':
                    for_linux = True
                    break
                if not for_linux:
                  continue

              if not locale in got_locales:
                print("Locale %s is missing from the source tarball" % locale)
                sys.exit(1)

        with open(os.path.join(application, 'config/version.txt'), 'r') as vf:
          upstream_version = re.sub(r'~$', '', re.sub(r'([0-9\.]*)(.*)', r'\1~\2', vf.read().strip(), 1))

        if version == None:
          version = upstream_version
          for line in CheckOutput(['hg', 'tip']).split('\n'):
            if line.startswith('changeset:'):
              rev = line.split()[1].split(':')[0].strip()
              changeset = line.split()[1].split(':')[1].strip()
              break

          u = urllib.request.urlopen('%s/pushlog?changeset=%s' % (repo, changeset))
          dom = xml.dom.minidom.parseString(u.read())
          t = time.strptime(dom.getElementsByTagName('updated')[0].firstChild.nodeValue.strip(), '%Y-%m-%dT%H:%M:%SZ')
          version += '~hg%s%s%sr%s' % ('%02d' % t.tm_year, '%02d' % t.tm_mon, '%02d' % t.tm_mday, rev)
          u.close()
        else:
          version = re.sub(r'~$', '', re.sub(r'([0-9\.]*)(.*)', r'\1~\2', version, 1))
          version += '+build%s' % build
          if not version.startswith(upstream_version):
            print("Version '%s' does not match upstream version '%s'" % (version, upstream_version))
            sys.exit(1)

        with open('BUILDID', 'w') as fd:
          fd.write(buildid)

        print('*** Debian package version is %s' % version)
        print('*** Packing tarball ***')
        with ScopedWorkingDirectory('..'):
          topsrcdir = '%s-%s' % (name, version)
          with ScopedRename(name, topsrcdir):
            env = {'XZ_OPT': '--threads=0'}
            args = ['tar', '-Jc', '--exclude=.hg', '--exclude=.hgignore', '--exclude=.hgtags', '--exclude=.svn']
            for exclude in settings['excludes']:
              args.append('--no-wildcards-match-slash') if exclude['wms'] == False else args.append('--wildcards-match-slash')
              args.append('--exclude')
              args.append(os.path.join(topsrcdir , exclude['path']))
            args.append('-f')
            args.append(os.path.join(orig_cwd, '%s_%s.orig.tar.xz' % (name, version)))
            for include in settings['includes']:
              args.append(os.path.join(topsrcdir, include))

            CheckCall(args, env=env)

def main():
  check_external_dependencies()
  creator = TarballCreator()
  creator.run()

if __name__ == '__main__':
  main()
