###############################################################################
#
# Package header
#
Summary:      A KNX daemon and tools
Name:         knxd
Version:      0.12.10
Release:      0%{?dist}
Group:        Applications/Interpreters
Source:       %{name}-%{version}.tar.gz
URL:          https://github.com/knxd/knxd/
Distribution: CentOS
Vendor:       smurf
License:      GPL
Packager:     Michael Kefeder <m.kefeder@gmail.com>
# libusb-1.0 is in libusbx package
Requires: libusbx
Requires: libev

%if 0%{?rhel} >= 7
BuildRequires: systemd
BuildRequires: systemd-devel
BuildRequires: libusbx-devel
BuildRequires: libev-devel
%endif

###############################################################################
%description
A KNX daemon and tools supporting it.

###############################################################################
# preparation for build
%prep
%autosetup -n %{name}-%{version}

###############################################################################
%build
%configure
%make_build

###############################################################################
%install
%make_install

###############################################################################
# what is being installed
%files
%defattr(-,root,root,-)

# /etc
%config(noreplace) %{_sysconfdir}/knxd.conf

# /usr/bin
%{_bindir}/eibnetdescribe
%{_bindir}/eibnetsearch
%{_bindir}/knxd
%{_bindir}/knxtool

# /usr/include
%{_includedir}/eibclient.h
%{_includedir}/eibloadresult.h
%{_includedir}/eibtypes.h

%{_libdir}/libeibclient.a
%{_libdir}/libeibclient.la
%{_libdir}/libeibclient.so
%{_libdir}/libeibclient.so.0
%{_libdir}/libeibclient.so.0.0.0

%{_unitdir}/knxd.service
%{_unitdir}/knxd.socket
%{_sysusersdir}/knxd.conf

%{_libexecdir}/knxd/busmonitor1
%{_libexecdir}/knxd/busmonitor2
%{_libexecdir}/knxd/busmonitor3
%{_libexecdir}/knxd/eibread-cgi
%{_libexecdir}/knxd/eibwrite-cgi
%{_libexecdir}/knxd/groupcacheclear
%{_libexecdir}/knxd/groupcachedisable
%{_libexecdir}/knxd/groupcacheenable
%{_libexecdir}/knxd/groupcachelastupdates
%{_libexecdir}/knxd/groupcacheread
%{_libexecdir}/knxd/groupcachereadsync
%{_libexecdir}/knxd/groupcacheremove
%{_libexecdir}/knxd/grouplisten
%{_libexecdir}/knxd/groupread
%{_libexecdir}/knxd/groupreadresponse
%{_libexecdir}/knxd/groupresponse
%{_libexecdir}/knxd/groupsocketlisten
%{_libexecdir}/knxd/groupsocketread
%{_libexecdir}/knxd/groupsocketswrite
%{_libexecdir}/knxd/groupsocketwrite
%{_libexecdir}/knxd/groupsresponse
%{_libexecdir}/knxd/groupswrite
%{_libexecdir}/knxd/groupwrite
%{_libexecdir}/knxd/madcread
%{_libexecdir}/knxd/maskver
%{_libexecdir}/knxd/mmaskver
%{_libexecdir}/knxd/mpeitype
%{_libexecdir}/knxd/mprogmodeoff
%{_libexecdir}/knxd/mprogmodeon
%{_libexecdir}/knxd/mprogmodestatus
%{_libexecdir}/knxd/mprogmodetoggle
%{_libexecdir}/knxd/mpropdesc
%{_libexecdir}/knxd/mpropread
%{_libexecdir}/knxd/mpropscan
%{_libexecdir}/knxd/mpropscanpoll
%{_libexecdir}/knxd/mpropwrite
%{_libexecdir}/knxd/mread
%{_libexecdir}/knxd/mrestart
%{_libexecdir}/knxd/msetkey
%{_libexecdir}/knxd/mwrite
%{_libexecdir}/knxd/mwriteplain
%{_libexecdir}/knxd/progmodeoff
%{_libexecdir}/knxd/progmodeon
%{_libexecdir}/knxd/progmodestatus
%{_libexecdir}/knxd/progmodetoggle
%{_libexecdir}/knxd/readindividual
%{_libexecdir}/knxd/vbusmonitor1
%{_libexecdir}/knxd/vbusmonitor1poll
%{_libexecdir}/knxd/vbusmonitor1time
%{_libexecdir}/knxd/vbusmonitor2
%{_libexecdir}/knxd/vbusmonitor3
%{_libexecdir}/knxd/writeaddress
%{_libexecdir}/knxd/xpropread
%{_libexecdir}/knxd/xpropwrite

# /usr/share/knxd/
%{_datarootdir}/knxd/EIBConnection.go
%{_datarootdir}/knxd/EIBConnection.cs
%{_datarootdir}/knxd/EIBConnection.lua
%{_datarootdir}/knxd/EIBConnection.pm
%{_datarootdir}/knxd/EIBConnection.py
%{_datarootdir}/knxd/EIBConnection.rb
%{_datarootdir}/knxd/EIBD.pas
%{_datarootdir}/knxd/eibclient.php
%{_datarootdir}/knxd/examples/busmonitor1.c
%{_datarootdir}/knxd/examples/busmonitor2.c
%{_datarootdir}/knxd/examples/busmonitor3.c
%{_datarootdir}/knxd/examples/eibread-cgi.c
%{_datarootdir}/knxd/examples/eibwrite-cgi.c
%{_datarootdir}/knxd/examples/groupcacheclear.c
%{_datarootdir}/knxd/examples/groupcachedisable.c
%{_datarootdir}/knxd/examples/groupcacheenable.c
%{_datarootdir}/knxd/examples/groupcachelastupdates.c
%{_datarootdir}/knxd/examples/groupcacheread.c
%{_datarootdir}/knxd/examples/groupcachereadsync.c
%{_datarootdir}/knxd/examples/groupcacheremove.c
%{_datarootdir}/knxd/examples/grouplisten.c
%{_datarootdir}/knxd/examples/groupread.c
%{_datarootdir}/knxd/examples/groupreadresponse.c
%{_datarootdir}/knxd/examples/groupresponse.c
%{_datarootdir}/knxd/examples/groupsocketlisten.c
%{_datarootdir}/knxd/examples/groupsocketread.c
%{_datarootdir}/knxd/examples/groupsocketswrite.c
%{_datarootdir}/knxd/examples/groupsocketwrite.c
%{_datarootdir}/knxd/examples/groupsresponse.c
%{_datarootdir}/knxd/examples/groupswrite.c
%{_datarootdir}/knxd/examples/groupwrite.c
%{_datarootdir}/knxd/examples/madcread.c
%{_datarootdir}/knxd/examples/maskver.c
%{_datarootdir}/knxd/examples/mmaskver.c
%{_datarootdir}/knxd/examples/mpeitype.c
%{_datarootdir}/knxd/examples/mprogmodeoff.c
%{_datarootdir}/knxd/examples/mprogmodeon.c
%{_datarootdir}/knxd/examples/mprogmodestatus.c
%{_datarootdir}/knxd/examples/mprogmodetoggle.c
%{_datarootdir}/knxd/examples/mpropdesc.c
%{_datarootdir}/knxd/examples/mpropread.c
%{_datarootdir}/knxd/examples/mpropscan.c
%{_datarootdir}/knxd/examples/mpropscanpoll.c
%{_datarootdir}/knxd/examples/mpropwrite.c
%{_datarootdir}/knxd/examples/mread.c
%{_datarootdir}/knxd/examples/mrestart.c
%{_datarootdir}/knxd/examples/msetkey.c
%{_datarootdir}/knxd/examples/mwrite.c
%{_datarootdir}/knxd/examples/mwriteplain.c
%{_datarootdir}/knxd/examples/progmodeoff.c
%{_datarootdir}/knxd/examples/progmodeon.c
%{_datarootdir}/knxd/examples/progmodestatus.c
%{_datarootdir}/knxd/examples/progmodetoggle.c
%{_datarootdir}/knxd/examples/readindividual.c
%{_datarootdir}/knxd/examples/vbusmonitor1.c
%{_datarootdir}/knxd/examples/vbusmonitor1poll.c
%{_datarootdir}/knxd/examples/vbusmonitor1time.c
%{_datarootdir}/knxd/examples/vbusmonitor2.c
%{_datarootdir}/knxd/examples/vbusmonitor3.c
%{_datarootdir}/knxd/examples/writeaddress.c
%{_datarootdir}/knxd/examples/xpropread.c
%{_datarootdir}/knxd/examples/xpropwrite.c

%exclude
%{_datarootdir}/knxd/EIBConnection.pyc
%{_datarootdir}/knxd/EIBConnection.pyo

###############################################################################
# preinstall
%pre
/usr/bin/getent group knxd > /dev/null || /usr/sbin/groupadd -r knxd
/usr/bin/getent passwd knxd > /dev/null || /usr/sbin/useradd -r -s /sbin/nologin -g knxd knxd

###############################################################################
# postinstall
%post

###############################################################################
# pre-remove script
%preun


###############################################################################
# post-remove script
%postun
if [ "$1" = "1" ]; then
    # this is an upgrade do nothing
    echo -n
elif [ "$1" = "0" ]; then
    # this is an uninstall remove user
    userdel --force knxd 2> /dev/null; true
fi

###############################################################################
# verify
%verifyscript

%changelog
* Thu Feb 09 2017 Michael Kefeder <m.kefeder@gmail.com> 0.12.10
- libev based release 0.12.10
- improved user add/del in spec file

* Mon Jan 09 2017 Michael Kefeder <m.kefeder@gmail.com> 0.11.18-0
- Initial spec file created and tested on CentOS 7
- remove users only on uninstall in %postun hook
- use normal specfile layout
