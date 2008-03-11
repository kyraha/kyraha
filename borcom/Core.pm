#!/usr/bin/perl

use strict;

package Core;
use Agent;

sub new {
  my $class = shift or return undef;
  my $cgi = shift || new CGI;
  my $this = bless {
    cgi => $cgi,
    dbh => DBI->connect( "DBI:mysql:database=test", "test", "test" ),
    cookies => [],
  }, $class;
  return $this;
}

sub cookies {
  my $this = shift or return undef;
  push @{$this->{'cookies'}}, @_ if @_;
  return $this->{'cookies'};
}

sub init_session {
  my $this = shift or return undef;
  if( $this->{'cgi'}->cookie('B') ) {
    # cookies are traveling, so we can use them
    $this->{'agent'} = new Agent( $this );
    if( $this->{'agent'} ) {
      $this->cookies( $this->{'cgi'}->cookie(-name => 'B', -value => $this->{'agent'}{'agent_id'}, -expires => '+3M' ) );
    }
    else {
      warn "Empty Agent\n";
    }
  }
  else {
    $this->cookies( $this->{'cgi'}->cookie(-name => 'B', -value => 'temporary' ) );
  }
}

1;

