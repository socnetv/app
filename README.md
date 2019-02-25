[![version](https://img.shields.io/github/release/socnetv/app.svg?logo=c%2B%2B)](https://github.com/socnetv/app/releases)
[![travis](https://img.shields.io/travis/socnetv/app.svg?branch=master&logo=travis)](https://travis-ci.org/socnetv/app)
[![appveyor](https://img.shields.io/appveyor/ci/oxy86/app.svg?logo=appveyor)](https://ci.appveyor.com/project/oxy86/app)
[![langs](https://img.shields.io/github/languages/top/socnetv/app.svg)](https://github.com/socnetv/app.git)
[![downloads](https://img.shields.io/github/downloads/socnetv/app/total.svg?logo=github)](https://socnetv.org/downloads)
[![license](https://img.shields.io/github/license/socnetv/app.svg)](https://github.com/socnetv/app/blob/master/COPYING)
[![website](https://img.shields.io/website-up-down-green-red/https/socnetv.org.svg)](https://socnetv.org)
====================================

[![socnetv](/src/images/socnetv.png)](https://socnetv.org) SocNetV - Social Network Visualizer
====================================



# 1. Overview

Social Network Visualizer (SocNetV) is a cross-platform, user-friendly free
software application for social network analysis and visualization.

With SocNetV you can:

- Draw social networks with a few clicks on a virtual canvas, load 
field data from a file in a supported format (GraphML, GraphViz, EdgeList, GML, 
Adjacency, Edgelist, Pajek, UCINET, etc.) or crawl the internet to create 
a social network of connected webpages.

- Edit actors and ties through point-and-click, analyse graph and social 
network properties, produce beautiful HTML reports and embed visualization 
layouts to the network.

Main features:

- Standard graph-theoretic and network cohesion metrics, such as density, 
diameter, geodesics and distances, connectedness, eccentricity, 
clustering coefficient, walks, reciprocity etc.

- Matrix routines: Adjacency plot, Laplacian matrix, Degree matrix, Cocitation, etc.

- Advanced structural measures for social network analysis
such as centrality and prestige indices (i.e. eigenvector and closeness centrality,
betweenness centrality, information centrality, power centrality, proximity and
pagerank prestige), 

- Community detection algorithms such as triad census, clique census, etc.

- Structural equivalence analysis, using hierarchical clustering, 
actor similarities and tie profile dissimilarities, pearson coefficients, etc.

- Random network creation, i.e. Erdos-Renyi, Watts-Strogatz, scale-free, lattice, etc.

- One-click recreation of well-known social network datasets such as Padgett's Florentine families.

- Layout algorithms based on either prominence indices (i.e. circular, level and nodal sizes 
by centrality score) or force-directed models (i.e. Kamada-Kawai, Fruchterman-Reingold, etc) 
for meaningful visualizations of your social network data.

- Multirelational loading and editing. You can load a network consisting of multiple 
relations or create a network on your own and add multiple relations to it. 

- Built-in web crawler, allowing you to automatically create networks from links found in a given initial URL.

- Comprehensive documentation, both online and while running the 
application, which explains each feature and algorithm of SocNetV in detail.

- Binary packages and installers for Windows, Linux and MacOS.

The program is Free Software, licensed under the GNU General Public License 3 (GPL3).
You can copy it as many times as you wish, or modify it, provided you keep the 
same license. 

The documentation is also Free, licensed under the Free Documentation License (FDL).


# 2. Availability & License

Official Website: https://socnetv.org
Email: info@socnetv.org

Author: Dimitris V. Kalamaras 
Blog:   https://dimitris.apeiro.gr

SocNetV is a cross-platform application, developed in C++ and Qt5.
Qt is an open source software development platform published under the GPL.

This means you can compile and run SocNetV on any Operating System supported by Qt.
See installation instructions in Section 3 below.

SocNetV is Free Software, distributed under the General Public Licence Version 3 
(see the COPYING file for details). 

The application is not a "finished" product. Therefore, there is no warranty of 
efficiency, correctness or usability. 

Nevertheless, we are looking forward to help you if you experience any problems 
with SocNetV. See Section 6 (bug reporting) below.



# 3. Installation

SocNetV is multi-platform, which means that it can be installed and run in every
Operating System supported by the Qt toolkit.

The project offers binaries and installers for the three major Operating Systems:
Windows, MacOS and Linux.

If there is no binary package for your OS, please download and compile the source code,
as explained further below.


## a) Install a binary package or installer (Linux/MacOS/Windows)

You can download an installer or a binary package for your Operating System from the
project's Downloads page: https://socnetv.org/downloads

Follow the instructions below to install it in your system.

### Install in Windows

To install SocNetV in Windows, download the latest SocNetV Windows installer from
the Downloads page, and double-click on the executable to start the installation.
Click Next and Accept the License (GPL) to install the program.

The program will be installed in the usual Windows Program Files directory and a new
Start Menu shortcut will be created.

Afterwards you can run the application from your Start menu.

### Install in MacOS

To install SocNetV in Mac, download the latest SocNetV MacOS installer from
the Downloads page, and double-click on the executable to start the installation.
The application will be installed in your Applications.

Please note that the first time you run SocNetV, you may need to double click on
the SocNetV application icon holding down the META key.

### Install in Linux

To run the latest version of SocNetV in Linux, download the latest Linux AppImage from
the project's Downloads page.

Make the file executable and double-click on it to run SocNetV.

Please note that a version of SocNetV is available in the repositories of most Linux distributions.
However that is not always the most recent and updated version.

To install the latest and greatest SocNetV version, users of openSUSE, Fedora and
Ubuntu/Debian are advised to add our own repositories to their systems.

In Debian and Ubuntu, add our repository and install SocNetV with these commands:

```
sudo add-apt-repository ppa:dimitris-kalamaras/ppa
sudo apt-get update
sudo apt-get install socnetv
```

In Fedora and openSUSE, choose and add the correct repository for your distro version 
from here: https://download.opensuse.org/repositories/home:/oxy86/ 

Once you add the repo, install SocNetV using the command (Fedora):
sudo yum install socnetv

or (openSUSE): 
```
sudo zypper in socnetv
```


## b) Compile from Source Code

To compile and install SocNetV from source you need the Qt5 toolkit
development libraries. Qt is an open source C++ toolkit, for Windows, Linux and MacOS.

Windows and MacOS users should download and install Qt from https://www.qt.io/developers

Linux users need to install the following packages:

openSUSE: libqt5-qtbase, libqt5-qtbase-devel, libQt5Charts5-devel, libqt5-qttools

Fedora: qt5-qtbase,qt5-qtbase-devel, qt5-qtcharts-devel, qt5-qttools

Debian: qt5-default, libqt5charts5-dev

Once you have Qt5 installed, you are ready to compile SocNetV from source.

Download the archive with the source code of the latest version from 
https://github.com/socnetv/app/releases/latest, i.e. SocNetV-2.x.tar.gz

Then type in the following commands in order to decompress the
SocNetV tarball and build it. Replace 2.X with the version you downloaded.

```
untar zxfv SocNetV-2.X.tar.gz
cd socnetv-2.X
qmake
make
sudo make install # or su -c 'make install'
```

Probably you have already done the first 2 steps, so just type in 'qmake' or 'qmake-qt5'.

When you finish compiling and installing, run the application typing:

```
socnetv
```

or go to Start Menu > Mathematics  > SocNetV.



# 4. Command Line Options
	
SocNetV is primarily a GUI program. Nevertheless, some command line options 
are available. Type:

./socnetv filename.net

to start socnetv with network named filename.net loaded.



# 5. Usage & documentation

For usage documentation, there are tooltips and What's This help messages
inside the application, when running SocNetV.

To see the full documentation, press F1 to display the SocNetV Manual.

The manual is also available online at the project's website.


# 6. Bug reporting & contact

If you have a bug report or a feature request, please file it
in our github issue tracker:
https://github.com/socnetv/app/issues

To contact us directly, send an email to: info@socnetv.org

