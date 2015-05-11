Summary: EPICS Data Interface
Name: edi
Version: 0.0.1
Release: 1
License: MICE
Group: PP/CAM
URL: hep0.shef.ac.uk:/scratch/robinson/software/edam
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
Requires: qt4
BuildRequires: epics-ca qt4-devel

%description
A server which allows reading and writing of EPICS process variables over a simple tcp connection
%prep
%setup -q

%build
make

%install
%makeinstall
mkdir -p $RPM_BUILD_ROOT/etc/sysconfig
/bin/cp sysconfig $RPM_BUILD_ROOT/etc/sysconfig/edi
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/init.d
/bin/cp init $RPM_BUILD_ROOT/etc/rc.d/init.d/edi

%files
%defattr(-,root,root)
%{_sbindir}/edid
%config(noreplace) /etc/sysconfig/edi
%config /etc/rc.d/init.d/edi

%clean
rm -rf $RPM_BUILD_ROOT

%post
if ! egrep '\<edi\>' /etc/services > /dev/null 2>&1 ; then
  echo "edi		4875/tcp" >> /etc/services
fi
/sbin/service edi start
/sbin/chkconfig --add edi

%preun
/sbin/chkconfig --del edi
/sbin/service edi stop

%changelog
* Tue Oct 15 2013 EPICS <epics@target2ctl> - 0.0.1-1
- added units EDU PV as standard

* Mon Feb 20 2012 Matt Robinson <m.robinson@sheffield.ac.uk> - 0.0.0-1
- Initial build.

