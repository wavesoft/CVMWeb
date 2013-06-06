#!/usr/bin/perl
use strict;
use warnings;
use Data::Dumper;

# Accepts one argument: the full path to a directory.
# Returns: A list of files that reside in that path.
sub get_js_list {
    my $path    = shift;

    opendir (DIR, $path)
        or die "Unable to open $path: $!";

    my @files =
        map { $path . '/' . $_ }
        grep { !/^\.{1,2}$/ }
        readdir (DIR);

    return
        grep { (/\.js$/) && (! -l $_) }
        map { -d $_ ? get_js_list ($_) : $_ }
        @files;
}

# Read file into buffer
sub read_file {
    my $file = shift;
    local $/;
    open(FILE, $file) or die "Can't read '$file' [$!]\n";  
    my $buf = <FILE>; 
    close (FILE);  
    return $buf;
}

# Concat everything
my @files = 
my $buffer = "";
$buffer .= read_file( $_ ) foreach (get_js_list('src'));

# Enclose it into a function and dump it into the final file
open  DUMP_FILE, ">~tmp-dump.js";
print DUMP_FILE "(function(global) {\n";
print DUMP_FILE $buffer."\n";
print DUMP_FILE "})(window);\n";
close DUMP_FILE;

# Compress
`java -jar bin/yuicompressor-2.4.8.jar -o cvmwebapi-1.0.js ~tmp-dump.js`;

# Remove temp file
unlink "~tmp-dump.js";
