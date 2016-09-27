#!/usr/bin/env perl

use strict;
use warnings;

my %nodes = ();
my %queue = ();
my %recs = ();

my $func = shift @ARGV;

while (my $in = <>) {
    chomp $in;

    if ($in =~ m/->/) {
        my @g = ($in =~ m/(Node\w+)/g);
        if ($nodes{$g[0]}) {
            if (not defined($queue{$g[1]})) {
                print "$in\n";
                if ($recs{$g[1]}) {
                    printf "$recs{$g[1]}\n";
                    delete $recs{$g[1]};
                } # if
            } # if
            if (not defined($nodes{$g[1]})) {
                $queue{$g[1]} = 1;
            } # if
            next;
        } # if
    } elsif ($in =~ m/shape/) {
        my @g = ($in =~ m/(Node\w+)/g);

        if ($in =~ m/${func}/o) {
            print "$in\n";
            $nodes{$g[0]} = 1;   
        } elsif ($queue{$g[0]}) {
            print "$in\n";
            $nodes{$g[0]} = 1;
            delete $queue{$g[0]};
        } else {
            $recs{$g[0]} = $in;
        } # if
    } else {
        print "$in\n";
    } # if
} # while
