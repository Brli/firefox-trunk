#!/usr/bin/perl

use strict;
use warnings;
use Cwd;

my $moz_supported_file;
my $lpom_dir;

my %blacklist;
my %locale2pkgname;
my %languages;

my $dir=getcwd;
chomp($dir);

my $file;

while (@ARGV) {
    my $arg = shift(@ARGV);
    if ($arg eq '-s') {
        $moz_supported_file = shift(@ARGV);
    } elsif ($arg eq '-l') {
        $lpom_dir = shift(@ARGV);
    } else {
        die "Unknown argument '$arg'";
    }
}

(defined($moz_supported_file)) || die "Need to specify a supported language list";

if (defined($lpom_dir)) {
    my $lang_file = "$lpom_dir/maps/languages";
    my $map_file = "$lpom_dir/maps/locale2pkgname";
    my $variant_file = "$lpom_dir/maps/variants";

    open($file, $lang_file);
    while (<$file>) {
        my $line = $_;
        chomp($line);
        my $langcode = $line;
        my $lang = $line;
        $langcode =~ s/([^:]*):*([^:]*)/$1/;
        $lang =~ s/([^:]*):*([^:]*)/$2/;
        $languages{$langcode} = $lang;
    }
    close($file);

    open($file, $map_file);
    while (<$file>) {
        my $line = $_;
        chomp($line);
        my $langcode = $line;
        my $pkgname = $line;
        $langcode =~ s/([^:]*):*([^:]*)/$1/;
        $pkgname =~ s/([^:]*):*([^:]*)/$2/;
        $locale2pkgname{$langcode} = $pkgname;
    }
    close($file);

    open($file, $variant_file);
    while (<$file>) {
        my $line = $_;
        chomp($line);
        my $langcode = $line;
        my $lang = $line;
        $langcode =~ s/([^:]*):*([^:]*)/$1/;
        $lang =~ s/([^:]*):*([^:]*)/$2/;
        $languages{$langcode} = $lang;
    }
    close($file);
}

open($file, "$dir/debian/locales-supported");
while (<$file>) {
    my $line = $_;
    chomp($line);
    my $langcode = $line;
    my $lang = $line;
    $langcode =~ s/([^:]*):*([^:]*):*([^:]*)/$2/;
    $lang =~ s/([^:]*):*([^:]*):*([^:]*)/$3/;
    $languages{$langcode} = $lang;
}
close($file);

open($file, "$dir/debian/locale-blacklist");
while (<$file>) {
    my $line = $_;
    chomp($line);
    $blacklist{$line} = 1;
}
close($file);

open($file, $moz_supported_file);
open(my $outfile, ">$dir/debian/locales-supported");
while (<$file>) {
    my $line = $_;
    chomp($line);
    my $langcode = $line;
    my $platforms = $line;
    $langcode =~ s/^([[:alnum:]\-]*)[[:space:]]*(.*)/$1/;
    $platforms =~ s/^([[:alnum:]\-]*)[[:space:]]*(.*)/$2/;
    if (($langcode eq "en-US") || (($platforms ne "") && (rindex($platforms, "linux") eq -1)) || (exists $blacklist{$langcode})) {
        next;
    }
    my $llangcode = lc($langcode);
    my $pkgname = $llangcode;
    if (exists $locale2pkgname{$llangcode}) {
        $pkgname = $locale2pkgname{$llangcode};
    }
    if (not exists $languages{$pkgname}) {
        if ($pkgname eq $llangcode) {
            $pkgname =~ s/\-.*//;
        }
        if (not exists $languages{$pkgname}) {
            die "No description for $pkgname";
        }
    }
    my $description = $languages{$pkgname};
    print $outfile "$langcode:$pkgname:$description\n";
}
close($file);
close($outfile);


