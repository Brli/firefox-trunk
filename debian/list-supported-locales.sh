#!/bin/sh

set -e

while getopts "m:l:" opt ; do
    case "$opt" in
        m)
            mozlocales=$OPTARG
            ;;
        l)
            l10ndir=$OPTARG
            ;;
    esac
done

if [ -z $mozlocales ] || [ -z $l10ndir ] ; then
    exit 1
fi

while read line ; do
    platforms=`echo $line | sed 's/^\([A-Za-z0-9\-]* \)\(.*\)/\2/'`
    language=`echo $line | sed 's/^\([A-Za-z0-9\-]*\)\(.*\)/\1/'`
    if [ "x$language" != "xen-US" ] && 
      ([ "x$platforms" = "x$language" ] || `echo $platforms | grep -q linux`) ; then
        if [ -f ${l10ndir}/${language}/toolkit/defines.inc ] ; then
            display_name=`cat ${l10ndir}/${language}/toolkit/defines.inc | 
              grep MOZ_LANG_TITLE | cut -d ' ' -f 3- | sed 's/\( (.*)\)//'`
            echo "$language;$display_name"
        fi
    fi
        
done < ${mozlocales}
