%define name    socnetv
%define version 0.6.0
%define release 1
%define prefix  /usr/local
%define lastrev %(LANG=en_US.UTF-8 && date +"%a %b %e %Y")

%define is_mandrake %(test -e /etc/mandrake-release && echo 1 || echo 0)
%define is_suse %(test -e /etc/SuSE-release && echo 1 || echo 0)
%define is_fedora %(test -e /etc/fedora-release && echo 1 || echo 0)
%define qmake qmake
%define lrelease lrelease


#BEGIN BUILDSERVICE COMMANDS
%if 0%{?fedora_version} != 0
%define is_suse 0
%define is_mandrake 0
%define is_fedora 1
%define breqr qt4-devel, qt-x11, desktop-file-utils
%define qmake /usr/bin/qmake-qt4
%define lrelease /usr/bin/lrelease-qt4
%endif


%if 0%{?suse_version} != 0
%define is_suse 1
%define is_mandrake 0
%define is_fedora 0
%define breqr libqt4-devel, libQtWebKit-devel, update-desktop-files
%define qmake /usr/bin/qmake
%define lrelease /usr/bin/lrelease
%endif  


%if 0%{?mandriva_version} != 0
%define is_suse 0
%define is_mandrake 1
%define is_fedora 0
%define breqr libqt4-devel, desktop-file-utils
%define qmake /usr/lib/qt4/bin/qmake
%define lrelease /usr/lib/qt4/bin/lrelease
%endif

#END BUILDSERVICE COMMANDS


%if %{is_fedora}
%define distr Fedora 	# %(cat /etc/fedora-release)
%define breqr qt4-devel, qt-x11, desktop-file-utils
%define qmake /usr/bin/qmake-qt4
%define lrelease /usr/bin/lrelease-qt4
%endif



%if %{is_suse}
%define distr SUSE	# %(head -1 /etc/SuSE-release)
%define breqr libqt4-devel, libQtWebKit-devel, update-desktop-files
%define qmake /usr/bin/qmake
%define lrelease /usr/bin/lrelease
%endif


%if %{is_mandrake}
%define distr Mandriva	# %(cat /etc/mandrake-release)
%define breqr libqt4-devel, desktop-file-utils
%define qmake /usr/lib/qt4/bin/qmake
%define lrelease /usr/lib/qt4/bin/lrelease
%endif


Name:		%{name}
Version:	%{version}
Release:	%{release}
Summary:	A Social Networks Analyser and Visualiser
License:	GPLv3
Group:		Productivity/Scientific/Math 
URL:		http://socnetv.sourceforge.net/
Vendor: 	Dimitris V. Kalamaras <dimitris.kalamaras@gmail.com>
#Packager:	Dimitris V. Kalamaras <dimitris.kalamaras@gmail.com>  # Removed for OBS warnings...
Source0:	SocNetV-%{version}.tar.bz2
Distribution:   %{distr}
Prefix:		%{prefix}
BuildRequires:	gcc-c++, %{breqr}
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-buildroot






%description
SocNetV (Social Networks Visualiser) is a flexible and 
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



%prep
%setup 
[ -f Makefile.cvs ] && %__make -f Makefile.cvs
chmod -R a-x+X COPYING ChangeLog INSTALL NEWS README TODO manual man nets src

%build
%configure
%__make

%install
%if %{is_fedora}
#desktop-file-validate %{buildroot}/%{_datadir}/applications/%{name}.desktop
#desktop-file-install --add-category="Math" --delete-original  --dir=%{buildroot}%{_datadir}/applications  %{buildroot}/%{_datadir}/applnk/Edutainment/%{name}.desktop
%endif



%makeinstall
rm -rf %{buildroot}/%{_datadir}/doc/%{name}

%clean
[ -d %{buildroot} -a "%{buildroot}" != "" ] && %__rm -rf  %{buildroot}




%files
%defattr(-,root,root)
%{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/pixmaps/%{name}.png
%{_mandir}/man1/*
%doc ChangeLog NEWS README TODO COPYING AUTHORS INSTALL manual





%changelog
* Mon May 27 2009 Dimitris Kalamaras <dimitris.kalamaras@gmail.com> - 0.6.0-1
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
