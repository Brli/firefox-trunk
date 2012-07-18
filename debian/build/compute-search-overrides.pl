#!/usr/bin/perl

use strict;
use warnings;
use XML::Simple;
use JSON;
use File::Basename;

sub check_matches {
    my $plugin = shift;
    my $conditions = shift;
    (ref($conditions) eq "HASH") or die "Ah, shit";

    my $matches = 0;
    if (ref($plugin) eq "ARRAY") {
        foreach my $subvalue (@{$plugin}) {
            $matches ||= check_matches($subvalue, $conditions);
        }
        return $matches;
    }

    (ref($plugin) eq "HASH") or return 0;
    (scalar(keys(%$conditions)) == 0) && return 1;

    foreach my $key (keys(%$conditions)) {
        $matches = 0;
        if (ref($conditions->{$key}) eq "HASH") {
            $matches = check_matches($plugin->{$key}, $conditions->{$key});
        } elsif (ref($conditions->{$key}) eq "ARRAY") {
            $matches = 1;
            foreach my $condition (@{$conditions->{$key}}) {
                $matches &&= check_matches($plugin->{$key}, $condition);
            }
        } else {
            if (exists($plugin->{$key}) and ref($plugin->{$key}) ne "HASH" and ref($plugin->{$key}) ne "ARRAY") {
                $matches = $conditions->{$key} =~ /[\+\?\.\*\^\$\(\)\[\]\{\}\|\\]*/ ? 
                            ($plugin->{$key} =~ /$conditions->{$key}/) :
                            ($plugin->{$key} eq $conditions->{$key});
            }
        }
        last if not $matches;
    }

    return $matches;
}

my $dir;
my $lang;

while (@ARGV) {
    my $arg = shift;
    if ($arg eq '-d') {
        $dir = shift;
    } elsif ($arg eq '-l') {
        $lang = shift;
    }
}

(defined($dir) && defined($lang)) || die "Missing options";

open(my $config_file, "debian/searchplugins/compute-overrides.json") or die "Cannot open config";
my $json;
while (<$config_file>) { $json .= $_; }
close($config_file);
my $config = JSON::decode_json($json);

my @plugins = <$dir/$lang/*.xml>;
foreach my $plugin (@plugins) {
    my $xml = new XML::Simple;
    my $data = $xml->XMLin($plugin, keyattr => []);

    my $matches = 0;
    foreach my $conditions (@{$config->{'matches'}}) {
        $matches = check_matches($data, $conditions);
        last if $matches;
    }
    foreach my $conditions (@{$config->{'ignore'}}) {
        $matches &&= not check_matches($data, $conditions);
        last if not $matches;
    }
    $matches && print basename($plugin) . "\n";
}
