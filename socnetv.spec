# spec file for package socnetv
#
# Copyright (c) 2014 Dimitris Kalamaras dimitris.kalamaras@gmail.com
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via http://bugs.opensuse.org/



%define name    socnetv
%define version 1.9
%define release 1
%define prefix  /usr/local
%define lastrev %(LANG=en_US.UTF-8 && date +"%a %b %e %Y")


%define is_suse %(test -e /etc/SuSE-release && echo 1 || echo 0)
%define is_fedora %(test -e /etc/fedora-release && echo 1 || echo 0)
%define qmake qmake
%define lrelease lrelease


#BEGIN BUILDSERVICE COMMANDS
%if 0%{?fedora_version}
%define is_suse 0
%define is_fedora 1
%endif


%if 0%{?suse_version}
%define is_suse 1
%define is_fedora 0
%endif


#END BUILDSERVICE COMMANDS


%if %{is_fedora}
%define distr Fedora
%define breqr qt5-qtbase,qt5-qtbase-devel, qt5-qttools, qt5-qtwebkit, qt5-qtwebkit-devel, fedora-release, desktop-file-utils
%define qmake /usr/bin/qmake-qt5
%define lrelease /usr/bin/lrelease
%endif



%if %{is_suse}
%define distr SUSE	# %(head -1 /etc/SuSE-release)
%define breqr libqt5-qtbase, libqt5-qtbase-devel, libqt5-qttools, libQt5WebKit5, libQt5WebKit5-devel, update-desktop-files
%define qmake /usr/bin/qmake-qt5
%define lrelease /usr/bin/lrelease
%endif




Name:		%{name}
Version:	%{version}
Release:	%{release}
Summary:	A Social Networks Analyser and Visualiser
License:	GPL-3.0	
Group:		Productivity/Scientific/Math 
URL:		http://socnetv.sourceforge.net/
Vendor: 	Dimitris V. Kalamaras <dimitris.kalamaras@gmail.com>
#Packager:	Dimitris V. Kalamaras <dimitris.kalamaras@gmail.com>  # Removed for OBS warnings...
Source0:	SocNetV-%{version}.tar.bz2
Distribution:   %{distr}
Prefix:		%{prefix}
BuildRequires:	gcc-c++, %{breqr}
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5PrintSupport)
BuildRequires:  pkgconfig(Qt5Widgets)
BuildRequires:  pkgconfig(Qt5Network)
#BuildRequires:  pkgconfig(Qt5WebKitWidgets)

BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-buildroot




#
#DESCRIPTION SECTION
#


%description
SocNetV (Social Network Visualiser) is a flexible and 
user-friendly tool for Social Networks Analysis and Visualisation. 
It lets you create new networks (graphs) with a few clicks on a 
virtual canvas or load networks of various formats (GraphViz, 
GraphML, Adjacency, Pajek, etc) and modify them to suit your needs.

The application can compute network properties, such as density, 
diameter and distances, as well as node and network centralities. 
Various layout algorithms (i.e. Spring-embedder, circular and in 
levels according to centralities) are supported for meaningful 
visualisations of your networks. Furthermore, simple random 
networks (lattice, same degree, etc) can be created. 

Author: Dimitris V. Kalamaras <dimitris.kalamaras@gmail.com>


#
#PREPARATION SECTION
#

%prep
%setup 
chmod -R a-x+X COPYING changelog.gz INSTALL NEWS README TODO manual man nets src
chmod 644 nets/*
find . -type f -name '*~' -delete
find . -type f -name '*.bak' -delete
rm -f config.log config.status Makefile socnetv.spec socnetv.mak
sed -i -e 's/INSTALLPATH = \//INSTALLPATH = ./g' socnetv.pro

#
#MAKE SECTION
#

%build
%{qmake}
%__make

#
#INSTALL SECTION
#

%install
%if %{is_fedora}
desktop-file-validate %{name}.desktop
#desktop-file-install --add-category="Math" --delete-original  --dir=%{buildroot}%{_datadir}/applications  %{buildroot}/%{_datadir}/applnk/Edutainment/%{name}.desktop
%endif



%makeinstall


mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_datadir}/pixmaps/
mkdir -p %{buildroot}%{_datadir}/applications/
mkdir -p %{buildroot}%{_mandir}/man1/
cp -r socnetv %{buildroot}%{_bindir}/%{name}
cp -r src/images/socnetv.png %{buildroot}%{_datadir}/pixmaps/%{name}.png
cp -r socnetv.desktop %{buildroot}%{_datadir}/applications/
cp -r man/socnetv.1.gz %{buildroot}%{_mandir}/man1

rm -rf %{buildroot}/%{_datadir}/doc/%{name}

%clean
[ -d %{buildroot} -a "%{buildroot}" != "" ] && %__rm -rf  %{buildroot}



#
#FILES SECTION
#

%files
%defattr(-,root,root)
%{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/pixmaps/%{name}.png
%{_mandir}/man1/*
%doc changelog.gz NEWS README TODO COPYING AUTHORS INSTALL manual



#
#CHANGELOG SECTION
#
%changelog
* Tue Jun 23 2015 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 1.9-1
- Synced with DEV version from upstream.
* Fri Jun 05 2015 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 1.8-1
- Synced with new stable version from upstream.
* Wed May 20 2015 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 1.7-1
- Synced with new stable version from upstream.
* Mon May 11 2015 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 1.6-1
- Synced with new stable version from upstream.
* Fri Oct 10 2014 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 1.5-1
- Synced with new stable version from upstream.
* Mon Sep 01 2014 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 1.4-1
- Synced with new stable version from upstream.
* Wed Aug 27 2014 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 1.3-1
- Synced with new stable version from upstream.
* Mon Aug 18 2014 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 1.2-1
- Synced with new stable version 1.2 from upstream.
* Fri Aug 01 2014 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 1.1-1
- Synced with new version from upstream.
* Thu Feb 27 2014 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 1.0-2
- Fixed spec for openSUSE
* Thu Feb 27 2014 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 1.0-1
- Synced with new version from upstream.
* Thu Oct 14 2010 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 0.90-1
- Synced with upstream.
* Thu Jan 28 2010 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 0.81-1
- Synced with upstream.
- Bugfixes for Windows version
* Sat Jan 09 2010 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 0.80-1
- Synced with upstream,
* Mon Jun 29 2009 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 0.70-1
- Synced with upstream
* Wed May 27 2009 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 0.6.0-1
- Synced with upstream
* Thu Feb 26 2009 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 0.52-1
- Synced with upstream.
- Bugfixes into .spec.in for RPMs (Fedora, openSUSE and Mandriva).
* Tue Feb 17 2009 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 0.51-3
- Bugfixes into .spec.in for Fedora and Mandriva. 
- RPM for Fedora
* Mon Feb 16 2009 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 0.51-2
- Minor changes to RPM
* Mon Feb 16 2009 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 0.51-1
- Updated to upstream version 0.51
* Fri Feb 13 2009 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 0.50-1
- Updated to upstream version 0.50
* Wed Jan 14 2009 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 0.49-2
- Package .spec fixes
* Tue Jan 13 2009 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 0.49-1
- Updated to 0.49
* Wed Sep 17 2008 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 0.48-1
- First RPM release
