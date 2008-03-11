#!/usr/bin/perl
# $Id:$

use strict;

package Core;
use Agent;
use Config::General;

sub new {
  my $class = shift or return undef;
  my $cgi = shift || new CGI;
  my $configfile = shift || ".htconfig";
  my $config = new Config::General( $configfile ) or return undef;
  my $this = bless {
    cgi => $cgi,
    config => { $config->getall },
    cookies => [],
  }, $class;
  $this->{'dbh'} = DBI->connect(
                     $this->{'config'}{'database'},
                     $this->{'config'}{'databaseuser'},
                     $this->{'config'}{'databasepass'},
                   ) or return undef;
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

