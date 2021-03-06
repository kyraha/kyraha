#!/usr/bin/perl
# $Id$

use strict;
use warnings;
use DBI;
use CGI;
use JSON;
use CFG;

my $q = CGI->new;

my $cookie_A;
my $name_nice = "";

my $dbh;
sub dbh {
    $dbh = DBI->connect(CFG->DB_DSN, CFG->DB_USER, CFG->DB_PASS)
            or die $DBI::errstr unless defined $dbh;
    return $dbh;
}

if( defined( my $session_id = $q->cookie('-name'=>'A') ) ) {

    my $session;

    if( $session_id eq 'new' ) {
        # New session requested
        $session_id = create_session();
        $name_nice = "New ". $session_id;
    }
    elsif( $session_id =~ /^[0-9A-G]{16}$/i ) {
        # Existing session if any

        $session = dbh->selectrow_hashref("select * from session where session_id='$session_id'")
            or dbh->err and die dbh->errstr;
        $name_nice = "Found ". $session_id;

        if( defined $session->{'session_id'} ) {
            dbh->do("update session set last_ts = now() where session_id = '$session_id'")
                or die dbh->errstr;
            $name_nice = "OK ". $session_id;
        }
        else {
            # Session not found, expired
            $session_id = create_session();
            $name_nice = "Not Found ". $session_id;
        }

    }

    $cookie_A = $q->cookie( -name => 'A', -value => $session_id, -expires => '+3M' );

}

#print $q->header('application/json');
print $q->header( -type=>'application/x-javascript', -cookie=>$cookie_A );

print "LoginNamespace.callback(";
print encode_json({
    name => $name_nice,
    auth => 0,
});
print ");";

sub create_session {
    my $session_id = unpack("H16",      # H = hex high nybble first
        pack( "N n2",               # N = 32 bit BE, n = 16 bit BE
        time, int(rand(65536)),int(rand(65536))));

    dbh->do("insert session(session_id,init_ts,last_ts) values('$session_id',now(),now())")
        or die dbh->errstr;
    return $session_id;
}

__END__

http://search.yahooapis.com/SiteExplorerService/V1/inlinkData?appid=3wEDxLHV34HvAU2lMnI51S4Qra5m.baugqoSv4gcRllqqVZm3UrMDZWToMivf5BJ3Mom&results=20&output=json&omit_inlinks=domain&callback=MyNamespace.callback&query=http://google.com
