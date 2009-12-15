'''firefox apport hook draft

/usr/share/apport/package-hooks/firefox-3.7.py

Appends to apport's firefox default report: the files pluginreg.dat and
profiles.ini, and also a summary of all the extensions loaded on each firefox
profile (the summary is the extension's name, it's version, and the id)
obtained by parsing each extension's install.rdf file.

Copyright (c) 2007: Hilario J. Montoliu <hmontoliu@gmail.com>

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.  See http://www.gnu.org/copyleft/gpl.html for
the full text of the license.
'''

import os
import ConfigParser
import cStringIO
from xml.dom import minidom

def extensions_ini_parser(extensions_ini_file):
    '''parses profile's extensions.ini file and returns a tuple:
    ((gre extensions, app extensions, local extensions), (gre themes, app
    themes, local themes))'''
    parser = ConfigParser.ConfigParser()
    parser.read(extensions_ini_file) 
    ext_ini_d = {}
    for section in parser.sections():
        section_gre, section_app, section_local, my_ext = [], [], [], ''
        for extension in parser.options(section):
            my_ext = parser.get(section, extension)
            if '/usr/lib/xulrunner' in my_ext:
                section_gre.append((my_ext))
            elif '/usr/lib/firefox' in my_ext:
                section_app.append((my_ext))
            else:
                section_local.append((my_ext))
        ext_ini_d[section] = (section_gre, section_app, section_local)
    return (ext_ini_d['ExtensionDirs'], ext_ini_d['ThemeDirs'])

def install_ini_parser(extension_path):
    '''parses each extension's install.rdf and returns string:
    extension name, its version and the id.'''
    rdf_file = os.path.join(extension_path, 'install.rdf')
    if not os.path.exists(extension_path):
        return '''  %s does not exist (old profile?)''' % extension_path
    refs_dict = {'em:version': '', 'em:id': '', 'em:name': ''}
    parse_err = '%s (Not Parsed)\n' % extension_path 
    dom_doc = minidom.parse(rdf_file)
    for key in refs_dict.keys():
        this_key = ''
        try:
            document_ref = dom_doc.getElementsByTagName('RDF:Description')[0].attributes
            this_key = document_ref[key].value
        except:
            try:
                document_ref = dom_doc.getElementsByTagName('Description')[0].attributes
                this_key = document_ref[key].value
            except: 
                try:
                    this_key = dom_doc.getElementsByTagName(key)[0].childNodes[0].data
                except:
                    return parse_err
        try: # avoid problems with encodings.
            print >> cStringIO.StringIO(), this_key
            refs_dict[key] = this_key            
        except UnicodeEncodeError:    
            refs_dict[key] = repr(this_key)
    return '''%(em:name)s (version: %(em:version)s) -\tid: %(em:id)s''' % refs_dict

def extension_summary_helper(extension_list, section_name, alt_output = 1):
    '''does some output proccessing for extensionSummary'''
    summary = ''
    if len(extension_list) > 0:
        summary += '''  %s:\n''' % section_name
        for extension in extension_list:
            summary += '''    %s\n''' % install_ini_parser(extension)
    else:
        if alt_output == 1: # if 0, don't output anything
            summary += '''  No %s in this Profile.\n''' % section_name 
    summary += '''\n'''
    return summary

def recent_kernlog(pattern):
    '''Extract recent messages from kern.log or message which match a regex.
       pattern should be a "re" object.  '''
    lines = ''
    if os.path.exists('/var/log/kern.log'):
        file = '/var/log/kern.log'
    elif os.path.exists('/var/log/messages'):
        file = '/var/log/messages'
    else:
        return lines

    for line in open(file):
        if pattern.search(line):
            lines += line
    return lines

def recent_auditlog(pattern):
    '''Extract recent messages from kern.log or message which match a regex.
       pattern should be a "re" object.  '''
    lines = ''
    if os.path.exists('/var/log/audit/audit.log'):
        file = '/var/log/audit/audit.log'
    else:
        return lines

    for line in open(file):
        if pattern.search(line):
            lines += line
    return lines

def add_info(report):
    '''adds hooked info into the apport report.'''
    config_dir = os.path.join(os.environ['HOME'], '.mozilla', 'firefox-3.7')
    profiles_d = {}
    # append profiles.ini file & parse it:
    profiles_ini = os.path.join(config_dir,'profiles.ini') 
    if os.path.exists(profiles_ini):
        report['profiles.ini'] = open(profiles_ini).read() 
        # parse profiles.ini: 
        profile_parser = ConfigParser.ConfigParser()
        profile_parser.read(profiles_ini)
        for section in profile_parser.sections():
            if profile_parser.has_option(section, 'Name') and profile_parser.has_option(section, 'Path'):
                if profile_parser.has_option(section, 'Default'):
                    is_default = profile_parser.get(section, 'Default')
                else:
                    is_default = 0
                profiles_d[profile_parser.get(section, 'Name')] = (os.path.join(config_dir, profile_parser.get(section, 'Path')), is_default)
    
    # summarize the extensions loaded on each profile (either global and local):
    # also append the pluginreg.dat file of the default profile (maybe in a
    # future append each profile's pluginreg.dat file)
    extensions_dict, themes_dict, extension_summary = {}, {}, ''
    for profile_name in profiles_d.keys():
        profile_path, is_default = profiles_d[profile_name]
        extensions_ini = os.path.join(profile_path, 'extensions.ini')
        pluginreg_dat = os.path.join(profile_path, 'pluginreg.dat')
        if os.path.exists(pluginreg_dat):
            if is_default == '1':
                report['default_profile_pluginreg.dat'] = open(pluginreg_dat).read()
            else:
                report['profile_%s_pluginreg.dat' % profile_name] = open(pluginreg_dat).read()
        if os.path.exists(extensions_ini):
            # attach each profile's extensions.ini too (not enabled).
            #report['extensions.ini (profile: %s)' % profile_name ] = open(extensions_ini).read()
            (extensions_dict['gre_extensions'], extensions_dict['app_extensions'], extensions_dict['local_extensions']), \
            (themes_dict['gre_theme'], themes_dict['app_theme'], themes_dict['local_theme']) = extensions_ini_parser(extensions_ini)
            if is_default == '1': 
                is_default_str = ''' (The Default):'''
            else: is_default_str = ''':'''
            extension_summary += '''Profile "%s"%s\n\n''' % (profile_name, is_default_str)
            extension_summary += extension_summary_helper(extensions_dict['gre_extensions'], 'GRE Extensions')
            extension_summary += extension_summary_helper(extensions_dict['app_extensions'], 'Application Extensions')
            extension_summary += extension_summary_helper(extensions_dict['local_extensions'], 'Local Extensions')
            extension_summary += extension_summary_helper(themes_dict['gre_theme'], 'GRE Theme', 0)
            extension_summary += extension_summary_helper(themes_dict['app_theme'], 'Application Theme', 0)
            extension_summary += extension_summary_helper(themes_dict['local_theme'], 'Local Theme', 0)
        wbuffer = cStringIO.StringIO() # it's needed for propper apport attachments
        print >> wbuffer, extension_summary
        wbuffer.seek(0)
    report['ExtensionSummary'] = wbuffer.read()

    # Get apparmor stuff if the profile isn't disabled. copied from
    # source_apparmor.py until apport runs hooks via attach_related_packages
    apparmor_disable_dir = "/etc/apparmor.d/disable"
    add_apparmor = True
    if os.path.isdir(apparmor_disable_dir):
        for f in os.listdir(apparmor_disable_dir):
            if f.startswith("usr.bin.firefox"):
                add_apparmor = False
                break
    if add_apparmor:
        attach_related_packages(report, ['apparmor', 'libapparmor1',
            'libapparmor-perl', 'apparmor-utils', 'auditd', 'libaudit0'])

        attach_file(report, '/proc/version_signature', 'ProcVersionSignature')
        attach_file(report, '/proc/cmdline', 'ProcCmdline')

        sec_re = re.compile('audit\(|apparmor|selinux|security', re.IGNORECASE)
        report['KernLog'] = recent_kernlog(sec_re)

        if os.path.exists("/var/log/audit"):
            # this needs to be run as root
            report['AuditLog'] = recent_auditlog(sec_re)

    # debug (comment on production)
    # return report

#### debug ####
# (uncomment the 'return report' at add_report())
if __name__ == "__main__":
    D = {}  
    REPORT = add_info(D)
    for KEY in REPORT.keys(): 
        print '''-------------------%s: ------------------\n''' % KEY, REPORT[KEY]
