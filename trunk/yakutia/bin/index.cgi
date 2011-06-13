#!/usr/bin/perl

use strict;
use warnings;
use Template;

print "Content-type: text/html;\n\n";

my $tt = new Template({
    INCLUDE_PATH => '../tt',
}) || die "$Template::ERROR\n";

$tt->process( 'front', { page_title => "Test front page", message => "blah blah blah" } )
    or die $tt->error(), "\n";


