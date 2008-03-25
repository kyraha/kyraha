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

