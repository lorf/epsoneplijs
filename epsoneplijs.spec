Summary: Ghostscript IJS Plugin for the Epson EPL-5700L/5800L/5900L printers
Name: epsoneplijs
Version: 0.3.0
%define LIBUSB_VERSION 0.1.7
Release: 1
Copyright: Copyright  (c) 2003 Hin-Tak Leung, Roberto Ragusa. Distribution and Use restricted.
Group: Applications/Graphics
Source0:  http://osdn.dl.sourceforge.net/sourceforge/epsonepl/epsoneplijs-%{version}.tgz
Source1:  http://osdn.dl.sourceforge.net/sourceforge/libusb/libusb-%{LIBUSB_VERSION}.tar.gz
URL: http://sourceforge.net/projects/epsonepl/
Vendor: http://sourceforge.net/projects/epsonepl/
Packager: Hin-Tak Leung, Roberto Ragusa
Requires: ghostscript >= 6.53
#Requires: ghostscript-fonts
Conflicts: ghostscript = 8.00
BuildRoot: /var/tmp/epsonepl-root

%description
Support for the Epson EPL-5700L/5800L/5900L/6100L printer family under linux and 
other unix-like systems. This effort is not edorsed by nor affliated with 
Epson. It is known to work for at least one user for each of 5700L, 5800L 
and 5900L. YMMV. 6100L tester needed.

%prep

%setup -a 1

%build

ln -s libusb-%{LIBUSB_VERSION} libusb 
perl -pi -e "s/STRICT_WIN32_COMPATIBILITY/__unix__/;" epl_config.h
./configure --with-kerneldevice --with-libusb
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/{bin,man,doc}
make install prefix=$RPM_BUILD_ROOT/usr
[ -f test5700lusb ] && install test5700lusb $RPM_BUILD_ROOT/usr/bin 
[ -f testlibusb   ] && install testlibusb   $RPM_BUILD_ROOT/usr/bin 
strip $RPM_BUILD_ROOT/usr/bin/*

%post

echo "Attempting to remove older installs in /usr/local/bin."
[ -f /usr/local/bin/ijs_server_epsonepl ] && rm -f /usr/local/bin/ijs_server_epsonepl
[ -f /usr/local/bin/ijs_client_epsonepl ] && rm -f /usr/local/bin/ijs_client_epsonepl
echo "Done."
echo ""
echo -n "Please check /usr/doc/epsoneplijs-" 
echo -n 0.3.0
echo " for tips on CUPS, foomatic, etc."
echo "Happy printing! Bye for now."
echo ""
exit 0

%files
%defattr(-, root, root)
%doc ChangeLog FAQ LIMITATIONS README* ps2epl epl_test* apsfilter cups epl_docs foomatic 
/usr/bin/*

%changelog
* Wed Feb  5 2003 Roberto Ragusa <rora@users.sourceforge.net> 
- Eliminated dependency on ghostscript-fonts (some distributions have
  a different spelling of the name of the package and we should not worry
  about fonts directly, just ask to have ghostscript).

* Wed Feb  5 2003 Hin-Tak Leung <htl10@users.sourceforge.net> 
- Merged USB-related changes

* Wed Feb  5 2003 Hin-Tak Leung <htl10@users.sourceforge.net> v0.2.2
- first ever release available in RPMS
