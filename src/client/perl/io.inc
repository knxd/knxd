/*
    EIBD client library
    Copyright (C) 2005-2011 Martin Koegler <mkoegler@auto.tuwien.ac.at>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    In addition to the permissions in the GNU General Public License, 
    you may link the compiled version of this file into combinations
    with other programs, and distribute those combinations without any 
    restriction coming from the use of this file. (The General Public 
    License restrictions do apply in other respects; for example, they 
    cover modification of the file, and distribution when not linked into 
    a combine executable.)

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

sub EIBSocketLocal {
    my $class = shift;
    my $self = {};
    bless ($self, $class);
    my ($path) = @_;
    socket($self->{fd}, PF_UNIX, SOCK_STREAM, 0);
    connect($self->{fd},  sockaddr_un($path)) or return undef;
    $self->{data} = {};
    $self->{readlen} = 0;
    return $self;
}

sub EIBSocketRemote {
    my $class = shift;
    my $self = {};
    bless ($self, $class);
    my ($ip, $port) = @_;
    my $host = gethostbyname($ip) or return undef;
    $port = $port || 6720;
    socket($self->{fd}, PF_INET, SOCK_STREAM, getprotobyname('tcp'));
    connect($self->{fd},  sockaddr_in($port, $host)) or return undef;
    setsockopt($self->{fd}, IPPROTO_TCP, &TCP_NODELAY, 1);
    $self->{data} = {};
    $self->{readlen} = 0;
    return $self;
}

sub EIBSocketURL {
    my $class = shift;
    my ($url) = @_;
    if ($url =~ /^local:(.*)$/ )
    {
	return $class->EIBSocketLocal($1);
    }
    if ($url =~ /^ip:([^:]+)(:[0-9]+)?$/ )
    {
	my $port = (defined $2 ? substr($2, 1) : undef);
	return $class->EIBSocketRemote($1, $port);
    }
    $! = &Errno::EINVAL;
    return undef;
}

sub EIBSocketClose_sync {
    my $self = shift;
    $self->EIBReset();
    return $self->EIBClose();
}

sub EIBClose {
    my $self = shift;
    close($self->{fd}) or return undef;
    $self->{fd} = undef;
    return 0;
}

sub _EIB_SendRequest {
    my $self = shift;
    my ($data) = @_;
    my $result;
    if (!defined $self->{fd})
    {
	$! = &Errno::ECONNRESET;
	return undef;
    }
    if (bytes::length($data) >= 0xffff || bytes::length($data) < 2)
    {
	$! = &Errno::EINVAL;
	return undef;
    }
    $result = pack ("n", bytes::length($data)). $data;
    defined(send($self->{fd}, $result, 0)) or return undef;
    return 0;
}

sub _EIB_CheckRequest {
    my $self = shift;
    my $buffer;
    my $i;
    my ($block) = @_;
    if (!defined $self->{fd})
    {
	$! = &Errno::ECONNRESET;
	return undef;
    }
    if ($self->{readlen} == 0)
    {
	$self->{data} = "";
	$self->{head} = "";
    }
    if ($self->{readlen} < 2)
    {
	fcntl($self->{fd}, F_SETFL, (fcntl($self->{fd}, F_GETFL, 0) & ~O_NONBLOCK) | ($block ? 0 : O_NONBLOCK ));
	if (!defined(recv($self->{fd}, $buffer, 2 - $self->{readlen}, 0)))
	{
	    $! = &Errno::ECONNRESET;
	    return -1;
	}
	$self->{head}.=$buffer;
	$self->{readlen}+=bytes::length($buffer);
    }
    if ($self->{readlen} < 2)
    {
	return 0;
    }
    $self->{datalen} = unpack("n", $self->{head});
    if ($self->{readlen} < $self->{datalen} + 2)
    {
	fcntl($self->{fd}, F_SETFL, (fcntl($self->{fd}, F_GETFL, 0) & ~O_NONBLOCK) | ($block ? 0 : O_NONBLOCK ));
	if (!defined(recv($self->{fd}, $buffer, $self->{datalen} + 2 - $self->{readlen}, 0)))
	{
	    $! = &Errno::ECONNRESET;
	    return undef;
	}
	$self->{data}.=$buffer;
	$self->{readlen}+=bytes::length($buffer);
    }
    return 0;
}

sub _EIB_Get_Request {
    my $self = shift;
    do {
	defined($self->_EIB_CheckRequest(true)) or return undef;
    } while ($self->{readlen} < 2 || ($self->{readlen} >= 2 && $self->{readlen} < $self->{datalen} + 2));
    $self->{readlen} = 0;
    return 0;
}

sub EIB_Poll_Complete {
    my $self = shift;
    defined($self->_EIB_CheckRequest(false)) or return undef;
    if ($self->{readlen} < 2 || ($self->{readlen} >= 2 && $self->{readlen} < $self->{datalen} + 2))
    {
	return 0;
    }
    return 1;
}

sub EIB_Poll_FD {
    my $self = shift;
    if (!defined $self->{fd})
    {
	$! = &Errno::EINVAL;
	return undef;
    }
    return $self->{fd};
}
