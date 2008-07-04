Summary: Ghostscript IJS Plugin for the Epson EPL-5700L/5800L/5900L/6100L/6200L printers
Name: epsoneplijs
Version: 0.4.1
%define LIBUSB_VERSION 0.1.7
%define LIBIEEE1284_VERSION 0.2.8
Release: 1
Copyright: Copyright  (c) 2003 Hin-Tak Leung, Roberto Ragusa. Distribution and Use restricted.
Group: Applications/Graphics
Source0:  http://osdn.dl.sourceforge.net/sourceforge/epsonepl/epsoneplijs-%{version}.tgz
Source1:  http://osdn.dl.sourceforge.net/sourceforge/libusb/libusb-%{LIBUSB_VERSION}.tar.gz
Source2:  http://osdn.dl.sourceforge.net/sourceforge/libieee1284/libieee1284-%{LIBIEEE1284_VERSION}.tar.bz2
URL: http://sourceforge.net/projects/epsonepl/
Vendor: http://sourceforge.net/projects/epsonepl/
Packager: Hin-Tak Leung, Roberto Ragusa
Requires: ghostscript >= 6.53
#Requires: ghostscript-fonts
Conflicts: ghostscript = 8.00, ghostscript = 8.14
BuildRoot: /var/tmp/epsonepl-root

%description
Support for the Epson EPL-5700L/5800L/5900L/6100L/6200L printer family under linux and 
other unix-like systems. This effort is not edorsed by nor affiliated with 
Epson. It is known to work for at least one user for each of 5700L, 5800L, 5900L,
and 6100L. 6100L and 6200L support is new. YMMV.

%prep

%setup -a 1 -a 2

%build

ln -s libusb-%{LIBUSB_VERSION} libusb 
ln -s libieee1284-%{LIBIEEE1284_VERSION} libieee1284 
#fixing ieee1284.h brokenness.
perl -i -e 'undef $/;$_ = <>;s/,\n\};/\};/g;s/,\s*\/\*.*?\*\/\n\};/\};/g;print;' libieee1284/include/ieee1284.h
perl -pi -e "s/STRICT_WIN32_COMPATIBILITY/__unix__/;" epl_config.h

## More flexible to just let it do its own job
# ./configure --with-kernelusb --with-kernel1284 --with-libusb --with-libieee1284 
./configure 
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/{bin,man,doc}
make install prefix=$RPM_BUILD_ROOT/usr

install ps2epl.pl $RPM_BUILD_ROOT/usr/bin

#[ -f test5700lusb ] && install test5700lusb $RPM_BUILD_ROOT/usr/bin

## The testlibusb binary doesn't belong to us and its installation 
## should be removed eventually, when we don't need debug info too often...

#[ -f testlibusb   ] && install testlibusb   $RPM_BUILD_ROOT/usr/bin
file $RPM_BUILD_ROOT/usr/bin/* | grep ELF | cut -d':' -f1 | xargs strip 

%clean

[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%post

echo "Attempting to remove older installs in /usr/local/bin."
[ -f /usr/local/bin/ijs_server_epsonepl ] && rm -f /usr/local/bin/ijs_server_epsonepl
[ -f /usr/local/bin/ijs_client_epsonepl ] && rm -f /usr/local/bin/ijs_client_epsonepl
echo "Done."
echo ""
echo -n "Please check /usr/doc/epsoneplijs-" 
echo -n 0.4.1
echo " for tips on CUPS, foomatic, etc."
echo "Happy printing! Bye for now."
echo ""
exit 0

%files
%defattr(-, root, root)
%doc ChangeLog FAQ LIMITATIONS README* ps2epl epl_test* apsfilter cups epl_docs foomatic gs-wts.ps 
/usr/bin/*

%changelog
* Sat Apr 26 2003 Hin-Tak Leung <htl10@users.sourceforge.net> 
- bidirectional IEEE1284 parallel port support

* Wed Feb  5 2003 Roberto Ragusa <rora@users.sourceforge.net> 
- Eliminated dependency on ghostscript-fonts (some distributions have
  a different spelling of the name of the package and we should not worry
  about fonts directly, just ask to have ghostscript).

* Wed Feb  5 2003 Hin-Tak Leung <htl10@users.sourceforge.net> 
- Merged USB-related changes

* Wed Feb  5 2003 Hin-Tak Leung <htl10@users.sourceforge.net> v0.2.2
- first ever release available in RPMS
