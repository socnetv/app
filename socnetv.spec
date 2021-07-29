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

set +x
echo "#############################"
echo "##### SET PLATFORM VARS ####"
echo "#############################"
set -x

%if %(if [[ "%{vendor}" == obs://* ]]; then echo 1; else echo 0; fi)
%define buildservice 1
%define usingbuildservice true
%define packingplatform %(echo openSUSE Build Service)
%else
%define usingbuildservice false
%define packingplatform %(. /etc/os-release 2>/dev/null; [ -n "$PRETTY_NAME" ] && echo "$PRETTY_NAME" || echo $HOSTNAME [`uname`])
%endif


set +x
echo "#############################"
echo "###### SET BUILD VARS #######"
echo "#############################"
set -x

%define name    socnetv
%define version 3.0-rc2
%define release 1

# Default qmake & lrelease (from linguist)
%define qmake qmake
%define lrelease lrelease

# Detect host Linux distribution
%if 0%{?fedora}
%define breqr qt5-qtbase,qt5-qtbase-devel, qt5-qttools
%define qmake /usr/bin/qmake-qt5
%define lrelease /usr/bin/lrelease-qt5
%endif

%if 0%{?suse_version}
%define breqr libqt5-qtbase, libqt5-qtbase-devel, libqt5-qtsvg-devel, libQt5Charts5-devel, libqt5-qttools
%define qmake /usr/bin/qmake-qt5
%define lrelease /usr/bin/lrelease-qt5
%endif

%if 0%{?mageia}
%define lrelease /usr/bin/lrelease-qt5
%endif


#
# Preamble section
#

Name:		%{name}
Version:	%{version}
Release:	%{release}{?dist}
Summary:	A Social Networks Analyser and Visualiser
License:	GPLv3
Group:		Productivity/Scientific/Math 
URL:		https://socnetv.org/
Source0:	https://github.com/socnetv/app/archive/v%{version}.tar.gz
BuildRequires:  make
BuildRequires:	gcc-c++
BuildRequires:	gzip

# BuildRequires:	%{breqr}

%if 0%{?suse_version}
BuildRequires:  libqt5-linguist
%endif

%if 0%{?fedora}
BuildRequires:	qt5-linguist
%endif

BuildRequires:	desktop-file-utils
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5PrintSupport)
BuildRequires:  pkgconfig(Qt5Widgets)
BuildRequires:  pkgconfig(Qt5Network)
# qt5-qtsvg-devel
BuildRequires:  pkgconfig(Qt5Svg)
# qt5-qtcharts-devel
BuildRequires:  pkgconfig(Qt5Charts)


#
# DESCRIPTION SECTION
#

%description
SocNetV (Social Network Visualizer) is a flexible, user-friendly 
free software application for social network analysis and 
visualisation. 

It lets you create new networks (graphs) with a few clicks on a 
virtual canvas or load networks of various formats (GraphViz, 
GraphML, Adjacency, Pajek, etc) and modify them to suit your needs.

The application computes graph theory metrics, such as density, 
diameter and distances (shortest paths) in directed and undirected,
weighted or non weighted graphs. It also computes node and 
network centrality and prestige indices, such as closeness, 
betweeness, eigenvector, information, power centralities 
and pagerank prestige. Community detection and structural 
equivalence algorithms are included, such as triad census, 
clique census, hierarchical cluster analysis, actor similarity 
and tie profile dissimilarities. 

Various layout algorithms (i.e. Spring-embedder, circular and in 
levels according to centrality or prestige) are supported for 
meaningful visualisations of your networks. 

Furthermore, SocNetV generates random networks using various models
such as Erdos-Renyi, Scale-Free, Small-World, d-regular etc.

The application also includes a simple web crawler to create 
a social network of web pages, where edges are the links between 
them. 

Author: Dimitris V. Kalamaras <dimitris.kalamaras@gmail.com>


set +x
echo "#############################"
echo "####### PREP SECTION ########"
echo "#############################"
set -x

%prep
echo "### running autosetup... ###"
# The autosetup macro is new, see: https://rpm.org/user_doc/autosetup.html
%autosetup -p 0 -n app-%{version}

echo "### unzip changelog ###"
gunzip changelog.gz
chmod -x changelog

echo "### Showing files ###"
find .



set +x
echo "#############################"
echo "####### BUILD SECTION #######"
echo "#############################"
set -x

%build
# Run lrelease to generate Qt message files from Qt Linguist translation files
lrelease socnetv.pro

# Run qmake
%{qmake}

# Run make to build the application
%__make %{?_smp_mflags}
# NOTE: Also available as the %make_build macro, but that is not available for openSUSE 13.2, Leap 42.2 and SLE 12 SP2 (rpm < 4.12).


set +x
echo "#############################"
echo "###### INSTALL SECTION ######"
echo "#############################"
set -x

%install
%{make_install} INSTALL_ROOT=%{buildroot}


set +x
echo "#############################"
echo "###### CHECK SECTION ######"
echo "#############################"
set -x

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/%{name}.desktop

set +x
echo "#############################"
echo "###### POST/POSTUN SECTION ##"
echo "#############################"
set -x

# Read more: https://docs.fedoraproject.org/en-US/packaging-guidelines/Scriptlets/

%post
# Scriptlet executed before the package is installed on the target system.
/usr/bin/update-desktop-database &> /dev/null || :


%postun
# Scriptlet executed just after the package is uninstalled from the target system.
/usr/bin/update-desktop-database &> /dev/null || :


set +x
echo "#############################"
echo "###### FILES SECTION ########"
echo "#############################"
set -x

%files
%defattr(-,root,root)
%license COPYING
%doc AUTHORS changelog NEWS README.md
%{_bindir}/%{name}
%{_datadir}/%{name}/%{name}_*.qm
%{_datadir}/applications/%{name}.desktop
%{_datadir}/pixmaps/%{name}.png
%{_datadir}/metainfo/%{name}.appdata.xml
%{_mandir}/man1/%{name}.1.gz



#
#CHANGELOG SECTION
#
%changelog
* Wed Jul 28 2021 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 3.0-rc2
- v3.0-rc2
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
