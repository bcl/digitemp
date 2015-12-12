%global with_libusb 1

Summary:           Dallas Semiconductor 1-wire device reading console application
Name:              digitemp
Version:           3.7.1
Release:           1%{?dist}
License:           GPLv2+
Group:             Applications/System
URL:               http://www.digitemp.com/
Source0:           https://github.com/bcl/%{name}/archive/v%{version}.tar.gz
%if %{with_libusb}
BuildRequires:     libusb-devel
%endif
BuildRoot:         %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%description
DigiTemp is a simple to use console application for reading values from
Dallas Semiconductor 1-wire devices. Its main use is for reading temperature
sensors, but it also reads counters and understands the 1-wire hubs with
devices on different branches of the network. DigiTemp now supports the
following 1-wire temperature sensors: DS18S20 (and DS1820), DS18B20, DS1822,
the DS2438 Smart Battery Monitor, DS2422 and DS2423 Counters, DS2409
MicroLAN Coupler (used in 1-wire hubs) and the AAG TAI-8540 humidity sensor.

%prep
%setup -q

%build
CFLAGS="$RPM_OPT_FLAGS -fPIE -DPIC"; export CFLAGS
make ds9097 %{?_smp_mflags}
make clean
make ds9097u %{?_smp_mflags}
%if %{with_libusb}
make clean
make ds2490 %{?_smp_mflags}
%endif

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT{%{_bindir},%{_mandir}/man1}
install -m 755 digitemp_DS9097 digitemp_DS9097U $RPM_BUILD_ROOT%{_bindir}
%if %{with_libusb}
install -m 755 digitemp_DS2490 $RPM_BUILD_ROOT%{_bindir}
%endif
install -p -m 644 %{name}.1 $RPM_BUILD_ROOT%{_mandir}/man1/%{name}.1

# Convert everything to UTF-8
iconv -f iso-8859-1 -t utf-8 -o ChangeLog.utf8 ChangeLog
touch -c -r ChangeLog ChangeLog.utf8; mv -f ChangeLog.utf8 ChangeLog

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc ChangeLog COPYING COPYRIGHT CREDITS FAQ README TODO
%doc dthowto.txt DS9097_Schematic.gif
%{_bindir}/%{name}*
%{_mandir}/man1/%{name}.*

%changelog
* Sat Dec 12 2015 Brian C. Lane <bcl@redhat.com> 3.7.1-1
- Update version to 3.7.1
- Fix version in digitemp.h

* Sat Dec 12 2015 Brian C. Lane <bcl@redhat.com> 3.7.0-1
- Updating to v3.7.0
- New upstream location at GitHub

* Thu Aug 28 2008 Brian C. Lane <bcl@brianlane.com> 3.6.0-1
- Releasing new version with these changes:
- Updated to the .spec file from Fedora9
- DS2490 now suppresses the 'Found usb ...' output
- Support for compiling under DARWIN
- Current reading added to DS2438
- DS28EA00 support added
- Tabbed output of 0.00 on CRC errors with log type -o2 or -o3
- New manpage from Debian digitemp maintainer

* Sun Feb 10 2008 Robert Scheck <robert@fedoraproject.org> 3.5.0-3
- Rebuilt against gcc 4.3

* Tue Aug 28 2007 Robert Scheck <robert@fedoraproject.org> 3.5.0-2
- Updated the license tag according to the guidelines

* Sun Jan 07 2007 Robert Scheck <robert@fedoraproject.org> 3.5.0-1
- Upgrade to 3.5.0
- Initial spec file for Fedora and Red Hat Enterprise Linux
