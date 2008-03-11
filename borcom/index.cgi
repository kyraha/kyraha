#!/usr/bin/perl

use strict;

use CGI;
use DBI;
use Core;


my $cgi = new CGI;
my $cfile = '.htconfig';
my $core = new Core( $cgi, $cfile );

unless( $core->init_session() ) {
  # Prepare something;
}



print $cgi->header( -cookie => $core->cookies );

print $cgi->h1('Just Ten Pieces for the World');

print $cgi->p( "DB connected." ) if $core->{'dbh'};
print $cgi->p( "Agent is OK." ) if $core->{'agent'};

print $cgi->p( "Params: <br />\n", join("<br />\n", map sprintf("%s = %s", $_, $cgi->param($_)), $cgi->param) );

print $cgi->p( "10 pieces website is comming soon. Here you will find ten pieces of something.
                Ten easy pieces for easy going people. It could be one hundred pieces, or it could be just two pieces
                but ten (10) pieces is just enough. Isn't it? It can be anything, jeans or music, cars or cookware,
                punch or crayons, bolts and nuts. Check back l8r. Take care. " );

my    $full_url      = $cgi->url(-full=>1);
my    $relative_url  = $cgi->url(-relative=>1);
my    $absolute_url  = $cgi->url(-absolute=>1);
my    $rel_with_path = $cgi->url(-relative=>1,-path_info=>1);
my    $abs_with_path = $cgi->url(-absolute=>1,-path_info=>1);
my    $url_with_path_and_query = $cgi->url(-path_info=>1,-query=>1);

print $cgi->p(
    "full_url: $full_url<br>\n",
    "relative_url: $relative_url<br>\n",
    "absolute_url: $absolute_url<br>\n",
    "rel_with_path: $rel_with_path<br>\n",
    "abs_with_path: $abs_with_path<br>\n",
    "url_with_path_and_query: $url_with_path_and_query<br>\n",
);

print $cgi->p( "Query string: ", $cgi->query_string );

print $cgi->a( {-href=>"http://www.yakutia.org/"}, "Yakutia, Sakha, contacts" );
print $cgi->p( "Cookies sent: ", join "<br>\n", map{ my $h=$_; join ",", map "$_=>$h->{$_}", keys %$h;} @{$core->cookies()} );

print "\n";

exit;

