###############################################################################
#
# Spec file for knxd 0.14.0 or later
#
Summary:      A KNX daemon and tools
Name:         knxd
Version:      0.16.0
Release:      0%{?dist}
Group:        Applications/Interpreters
Source:       %{name}-%{version}.tar.gz
URL:          https://github.com/knxd/knxd/
Distribution: CentOS
Vendor:       smurf
License:      GPL
Packager:     Michael Kefeder <m.kefeder@gmail.com>

%if 0%{?rhel} >= 7 || %{defined fedora}
# libusb-1.0 is in libusbx package on Redhat based distros
Requires: libusbx
%endif
%if %{defined suse_version}
Requires: libusb-1_0
%endif
Requires: systemd
Requires: libev

%if %{defined fedora}
Requires: fmt
BuildRequires: fmt-devel
%endif
BuildRequires: systemd
BuildRequires: systemd-devel
BuildRequires: libev-devel
BuildRequires: cmake
%if 0%{?rhel} >= 7 || %{defined fedora}
BuildRequires: libusbx-devel
%endif
%if %{defined suse_version}
BuildRequires: systemd-rpm-macros
%{?systemd_requires}
BuildRequires: libusb-1_0-devel
%endif
# Opensuse 13 systemd-rpm-macros does not define that!?
%if %{undefined _sysusersdir} && %{defined suse_version}
%define _sysusersdir /usr/lib/sysusers.d
%endif

###############################################################################
%description
A KNX daemon and tools supporting it.

###############################################################################
# preparation for build
%prep
#%autosetup -n %{name}-%{version}
%setup -q

###############################################################################
%build
%if %{defined fedora}
mkdir build
cd build
%cmake -DCMAKE_INSTALL_LIBDIR:PATH=%{_libdir} -DCMAKE_INSTALL_SYSCONFDIR:PATH=%{_sysconfdir} ..
%else
%cmake -DCMAKE_INSTALL_SYSCONFDIR:PATH=%{_sysconfdir} -DCMAKE_INSTALL_LIBEXECDIR:PATH="lib/"
%endif%
%if %{defined suse_version}
  make
%else
  %make_build
%endif

###############################################################################
%install
cd build
%make_install

###############################################################################
# what is being installed
%files
%defattr(-,root,root,-)

# /etc
%config(noreplace) %{_sysconfdir}/knxd.conf

# /usr/bin
%{_bindir}/knxd
%{_bindir}/knxtool
%{_bindir}/findknxusb

# /usr/include
%{_includedir}/eibclient.h
%{_includedir}/eibloadresult.h
%{_includedir}/eibtypes.h

%{_libdir}/libeibclient.a
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
%{_libexecdir}/knxd_args
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

%if %{undefined suse_version}
%exclude
%{_datarootdir}/knxd/EIBConnection.pyc
%{_datarootdir}/knxd/EIBConnection.pyo
%endif

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
* Tue Jun 13 2017 Michael Kefeder <m.kefeder@gmail.com> 0.16.0
- moved to cmake build
- builds on Fedora 25
- builds on openSUSE LEAP 43.2

* Wed May 17 2017 Michael Kefeder <m.kefeder@gmail.com> 0.14.15
- builds on Fedora 25
- builds on openSUSE LEAP 43.2
