#
# spec file for package socnetv
#
# Dimitris Kalamaras dimitris.kalamaras@gmail.com
#
# Refer to the following for more info on .spec file syntax:
#
#   http://www.rpm.org/max-rpm/
#   http://www.rpm.org/max-rpm-snapshot/	(Updated version)
#   https://docs.fedoraproject.org/en-US/Fedora_Draft_Documentation/0.1/html/RPM_Guide/
#   https://rpm-packaging-guide.github.io/
#
# See also http://www.rpm.org
#

### Define our defaults, they will be overriden in the distro detection below.
%define qmake /usr/bin/qmake-qt5
%define lrelease /usr/bin/lrelease-qt5

### Detect host Linux distribution and update defines.
%if 0%{?fedora} || 0%{?rhel_version} || 0%{?centos_version}
%define qmake /usr/bin/qmake-qt5
%define lrelease /usr/bin/lrelease-qt5
%endif

%if 0%{?suse_version}
%define qmake /usr/bin/qmake-qt5
%define lrelease /usr/bin/lrelease-qt5
%endif

%if 0%{?mageia}
%define qmake /usr/bin/qmake-qt5
%define lrelease /usr/bin/lrelease-qt5
%endif

Name:		socnetv
Version:	3.0.2
Release:	1{?dist}
Summary:	A Social Networks Analyser and Visualiser
License:	GPL-3.0-or-later
Group:		Productivity/Scientific/Math 
URL:		https://socnetv.org/
Source0:	app-%{version}.tar.gz
BuildRequires:  make
BuildRequires:	gcc-c++
BuildRequires:	gzip

%if 0%{?suse_version}
BuildRequires:  libqt5-linguist
%endif

%if 0%{?fedora} || 0%{?rhel_version} || 0%{?centos_version}
BuildRequires:	qt5-linguist
BuildRequires:  glibc-all-langpacks
%endif


BuildRequires:	desktop-file-utils
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5PrintSupport)
BuildRequires:  pkgconfig(Qt5Widgets)
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  pkgconfig(Qt5Xml)
# qt5-qtsvg-devel
BuildRequires:  pkgconfig(Qt5Svg)
# qt5-qtcharts-devel
BuildRequires:  pkgconfig(Qt5Charts)


%description
SocNetV (Social Network Visualizer) is a flexible, user-friendly 
free software application for social network analysis and 
visualisation.

### Added to avoid "empty files file debugsourcefiles.list " error
### see https://en.opensuse.org/Fedora_packaging
### Another solution would be to add CONFIG += force_debug_info in qmake
%global debug_package %{nil}

%prep
### Runn autosetup. The autosetup macro is new, see: https://rpm.org/user_doc/autosetup.html
%autosetup -p 0 -n app-%{version}

### Unzip changelog
gunzip changelog.gz
chmod -x changelog

### Debugging: Show files
pwd
find .


%build
# Run lrelease to generate Qt message files from Qt Linguist translation files
lrelease-qt5 socnetv.pro

### Run qmake
qmake-qt5 CONFIG+=release

### Run make to build the application
%__make %{?_smp_mflags}
# NOTE: Also available as the make_build macro, but that is not available for openSUSE 13.2, Leap 42.2 and SLE 12 SP2 (rpm < 4.12).

%install
%{make_install} INSTALL_ROOT=%{buildroot}

### Debugging: Show where we are and show files in build root.
pwd
find %{buildroot}

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/%{name}.desktop

### Debugging: show where we are again
pwd
pwd

### Run post install and post uninstall Scriptlets
### Read more: https://docs.fedoraproject.org/en-US/packaging-guidelines/Scriptlets/

%post
### Scriptlet executed before the package is installed on the target system.
/usr/bin/update-desktop-database &> /dev/null || :


%postun
### Scriptlet executed just after the package is uninstalled from the target system.
/usr/bin/update-desktop-database &> /dev/null || :

### Debugging: show where we are for a last time
pwd
pwd
pwd


%files
%defattr(-,root,root)
%dir /usr/share/socnetv
%license COPYING
%doc AUTHORS NEWS README.md changelog
%{_bindir}/%{name}
%{_datadir}/%{name}/%{name}_*.qm
%{_datadir}/applications/%{name}.desktop
%{_datadir}/pixmaps/%{name}.png
%{_datadir}/metainfo/%{name}.appdata.xml
%{_mandir}/man1/%{name}.1.gz



###
### CHANGELOG SECTION
###
%changelog
* Fri Jul 30 2021 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 3.0.4-1
- Upstream v3.0.4
* Fri Jul 30 2021 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 3.0.3-1
- Upstream v3.0.3
* Fri Jul 30 2021 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 3.0.2-1
- Upstream v3.0.2
* Fri Jul 30 2021 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 3.0.1-1
- Upstream v3.0.1
* Fri Jul 30 2021 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 3.0-1
- Upstream v3.0
* Mon Jun 14 2021 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 2.9-1
- Upstream v2.9
* Sun Jan 03 2021 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 2.8-1
- Upstream v2.8
* Mon Dec 28 2020 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 2.7-1
- Sync with upstream v2.7
* Mon Dec 28 2020 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 2.6-2
- Synced with fixed upstream development 2.6 version
* Mon Dec 28 2020 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 2.6-1
- Synced with upstream development 2.6 version
* Fri Mar 8 2019 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 2.5-3
- Synced with new 2.5 version from upstream
* Wed Feb 20 2019 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 2.5-2
- Synced with new beta2 version from upstream
* Tue Feb 19 2019 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 2.5-1
- Synced with new beta version from upstream
* Wed Feb 28 2018 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 2.4-2
- Synced with fixed table version from upstream
* Tue Feb 27 2018 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 2.4-1
- Synced with new stable version from upstream
* Wed Jul 5 2017 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 2.3-1
- Synced with new stable version from upstream
* Sat Jan 21 2017 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 2.2-1
- Synced with new stable version from upstream
* Wed Sep 28 2016 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 2.1-1
- Synced with new stable version from upstream.
* Tue Sep 13 2016 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 2.0-2
- Spec patch for Buildservice
* Mon Sep 12 2016 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 2.0-1
- Synced with new stable version from upstream.
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
