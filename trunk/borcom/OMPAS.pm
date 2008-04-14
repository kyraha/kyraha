# 
# The MIT License
# 
# Copyright (c) 2008 Michael Kyraha, http://michael.kyraha.com/
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
# 

use strict;

package OMPAS;
use OMPAS::Auth::Agent;
use Config::General;
use CGI;
use DBI;

sub new {
  my $class = shift or return undef;
  my $this = bless {
    config => {},
    cookies => [],
  }, $class;
  $this->init(@_) if @_;
  return $this;
}

sub init {
  my $this = shift or return undef;
  my $param = shift || {};
  my $configfile = $param->{'configfile'} || ".htconfig";
  my $config = new Config::General( $configfile ) or return $this->errmess('Can not init config');
  $this->{'config'} = { $config->getall };
  $this->{'cgi'} = $param->{'cgi'} || new CGI or return $this->errmess('Can not initialize CGI');
  $this->{'dbh'} = DBI->connect(
                     $this->{'config'}{'database'},
                     $this->{'config'}{'databaseuser'},
                     $this->{'config'}{'databasepass'},
                   ) or return $this->errmess('Can not init DB handler: %s', $DBI::errstr);
  $this->init_session() or return $this->errmess('Can not init a session');
  return $this;
}

sub errmess {
  my $this = shift or return undef;
  return $this->{'errmess'} unless @_;
  my $this->{'errmess'} = @_ == 1 ? shift : sprintf @_;
  return undef; # usually methods are about to set errmess and return undef
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
    $this->{'agent'} = new OMPAS::Auth::Agent( $this );
    if( $this->{'agent'} ) {
      $this->cookies( $this->{'cgi'}->cookie(-name => 'B', -value => $this->{'agent'}{'agent_id'}, -expires => '+3M' ) );
    }
    else {
      return $this->errmess( "Empty OMPAS::Auth::Agent" );
    }
  }
  else {
    $this->cookies( $this->{'cgi'}->cookie(-name => 'B', -value => 'temporary' ) );
  }
}

1;

