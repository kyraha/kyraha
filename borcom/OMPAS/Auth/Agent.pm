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

package OMPAS::Auth::Agent;
use strict;

sub new {
  my $class = shift or return undef;
  my $core = shift or return undef;
 
  my $this = bless { core => $core }, $class;
  return $this->init();
}

sub init {
  my $this = shift or return undef;
  my $id = $this->{'core'}{'cgi'}->cookie('B');
  return $this if $this->restore($id) or $this->create;
  return undef; # shouldn't ever happen
}

sub restore {
  my $this = shift or return undef;
  my $id = shift;
  if( $id =~ /^\d+$/ ) {
    warn "DEBUG: Select agent with id = $id\n";
    ( $this->{'agent_id'}, $this->{'start_date'}, $this->{'last_date'}, $this->{'status'} )
      = $this->{'core'}{'dbh'}->selectrow_array(q{
              SELECT agent_id,start_date,last_date,status
	      FROM agent
	      WHERE agent_id=?}, undef, $id ) or return undef;
  }
  return $this->{'agent_id'};
}

sub create {
  my $this = shift or return undef;
  my ( $id, $rows );
  my $tries = 5;
  while( $tries-- ) {
    $id = sprintf "%d%06d", time, int rand 100000;
    warn "DEBUG: Create agent id=$id\n";
    $rows = $this->{'core'}{'dbh'}->do( q{ INSERT INTO agent(agent_id,last_date,status) values(?,now(),0) }, undef, $id );
    if( $rows == 1 ) {
      $this->{'agent_id'} = $id;
      $this->{'status'} = 0;
      return $this->{'agent_id'};
    }
    else {
      warn sprintf "Insert failure. %s\n", $this->{'core'}{'dbh'}->errstr;
    }
  }
  return undef;
}


1;

