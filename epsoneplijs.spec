Summary: Ghostscript IJS Plugin for the Epson EPL-5700L/5800L/5900L printers
Name: epsoneplijs
Version: 0.2.2
Release: 1
Copyright: Copyright  (c) 2003 Hin-Tak Leung, Roberto Ragusa. Distribution and Use restricted.
Group: Applications/Graphics
Source0:  http://osdn.dl.sourceforge.net/sourceforge/epsonepl/epsoneplijs-0.2.2.tgz
URL: http://sourceforge.net/projects/epsonepl/
Vendor: http://sourceforge.net/projects/epsonepl/
Packager: Hin-Tak Leung, Roberto Ragusa
Requires: ghostscript >= 6.53
Requires: ghostscript-fonts
Conflicts: ghostscript = 8.00
BuildRoot: /var/tmp/epsonepl-root

%description
Support for the Epson EPL-5700L/5800L/5900L printer family under linux and 
other unix-like systems. This effort is not edorsed by nor affliated with 
Epson. It is known to work for at least one user for each of 5700L, 5800L 
and 5900L. YMMV.
%prep

%setup 

%build

./configure
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/{bin,man,doc}
make install prefix=$RPM_BUILD_ROOT/usr
# install test5700lusb testlibusb $RPM_BUILD_ROOT/usr/bin
strip $RPM_BUILD_ROOT/usr/bin/*

%post

echo "Attempting to remove older installs in /usr/local/bin."
[ -f /usr/local/bin/ijs_server_epsonepl ] && rm -f /usr/local/bin/ijs_server_epsonepl
[ -f /usr/local/bin/ijs_client_epsonepl ] && rm -f /usr/local/bin/ijs_client_epsonepl
echo "Done."
echo ""
echo -n "Please check /usr/doc/epsoneplijs-" 
echo -n 0.2.2
echo " for tips on CUPS, foomatic, etc."
echo "Happy printing! Bye for now."
echo ""
exit 0

%files
%defattr(-, root, root)
%doc ChangeLog FAQ LIMITATIONS README TODO ps2epl epl_test.ps cups epl_docs foomatic 
/usr/bin/*

%changelog
* Wed Feb  5 2003 Hin-Tak Leung <htl10@users.sourceforge.net> v0.2.2
- first ever release available in RPMS