Summary: Ctwm is a window manager for the X Window System.
Name: ctwm
Version: 3.7
%define versuffix -beta4
Release: 1
Source: http://ctwm.free.lp.se/preview/%{name}-%{version}%{versuffix}.tar.gz
Copyright: MIT
Group: User Interface/X
Packager: Richard Levitte <richard@levitte.org>
BuildRoot: /var/tmp/%{name}-%{version}%{versuffix}-buildroot

%description
Ctwm is a window manager for the X Window System.  It provides
titlebars, shaped windows, virtual screens (workspaces), several forms
of icon management, user-defined macro functions, click-to-type and
pointer-driven keyboard focus, and user-specified key and pointer
button bindings.  It is actually twm (Tab Window Manager) from the MIT
X11 distribution slightly modified to accommodate the use of several
virtual screens (workspaces). It is heavily inspired from the
Hewlett-Packard vuewm window manager.  In addition, ctwm can use
coloured, shaped icons and background root pixmaps in XPM and JPG format,
as well as any format understood by the imconv package [from the
San Diego Supercomputer Center] and xwd files.  Ctwm can be compiled
to use both, either or none of the above icon/pixmap formats.

%prep

%setup -n %{name}-%{version}%{versuffix}

%build
cp Imakefile.local-template Imakefile.local
xmkmf
make

%install
rm -fr $RPM_BUILD_ROOT

# Call the man page something a little more Unixly.
mkdir -p $RPM_BUILD_ROOT/usr/X11R6/man/man1

# RedHat-ish OSes have the configuration file in /etc/X11/ctwm,
# possibly with a symlink /usr/X11R6/lib/X11/ctwm pointing at it.
make install \
    DESTDIR=$RPM_BUILD_ROOT \
    PIXMAPDIR=/usr/X11R6/include/X11/pixmaps \
    CONFDIR=/etc/X11/ctwm
mkdir -p $RPM_BUILD_ROOT/usr/X11R6/lib/X11
ln -s /etc/X11/ctwm $RPM_BUILD_ROOT/usr/X11R6/lib/X11/ctwm

# Install the manual page separately.
install -c -m 0644 ctwm.man $RPM_BUILD_ROOT/usr/X11R6/man/man1/ctwm.1x

%clean
rm -fr $RPM_BUILD_ROOT

%files
%defattr(0644,root,root,0755)
%doc README CHANGES PROBLEMS README.gnome TODO.gnome

%attr(0755,root,root) /usr/X11R6/bin/ctwm
%attr(0644,root,root) /usr/X11R6/man/man1/ctwm.1x.gz

/usr/X11R6/lib/X11/ctwm
%config %attr(0644,root,root) /etc/X11/ctwm/system.ctwmrc
%attr(0644,root,root) /usr/X11R6/include/X11/pixmaps/*

%changelog
* Tue May  3 2005 Richard Levitte <richard@levitte.org>
- Received the original from Johan Vromans.  Adjusted it to become
  an official .spec file.
* Wed May  4 2005 Richard Levitte <richard@levitte.org>
- Changed some directory specifications to RedHat-ish standards.
