# SocNetV

[![version](https://img.shields.io/github/release/socnetv/app.svg?logo=c%2B%2B)](https://github.com/socnetv/app/releases)
[![Build Status GitHub Actions](https://github.com/socnetv/app/actions/workflows/build-ci.yml/badge.svg)](https://github.com/socnetv/app/actions/workflows/build-ci.yml)
[![Build Status in Travis (.com)](https://img.shields.io/travis/com/socnetv/app/master?logo=travis)](https://travis-ci.com/github/socnetv/app)
[![Build Status in Appveyor](https://img.shields.io/appveyor/ci/oxy86/app.svg?logo=appveyor)](https://ci.appveyor.com/project/oxy86/app)
[![langs](https://img.shields.io/github/languages/top/socnetv/app.svg)](https://github.com/socnetv/app.git)
[![downloads](https://img.shields.io/github/downloads/socnetv/app/total.svg?logo=github)](https://socnetv.org/downloads)
[![license](https://img.shields.io/github/license/socnetv/app.svg)](https://github.com/socnetv/app/blob/master/COPYING)
[![website](https://img.shields.io/website-up-down-green-red/https/socnetv.org.svg)](https://socnetv.org)

[![socnetv](/src/images/socnetv.png)](https://socnetv.org) SocNetV - Social Network Visualizer

---

## 1. Overview

Social Network Visualizer (SocNetV) is a cross-platform, user-friendly free software application for social network analysis and visualization.

With SocNetV you can:

- Draw social networks with a few clicks on a virtual canvas, load your
field data from a file in a supported format (GraphML, GraphViz, EdgeList, GML, Adjacency, Edgelist, Pajek, UCINET, etc.), automatically recreate famous data sets or crawl the internet to create a social network of connected webpages.

- Edit actors and ties through point-and-click, analyse graph and social network properties, produce beautiful HTML reports and embed visualization layouts to the network.

[![socnetv](https://socnetv.org/data/uploads/screenshots/25/socnetv-25-padget-power-centrality-size-distribution.png)](https://socnetv.org)

## 2. Features

- Standard graph-theoretic and network cohesion metrics, such as density, diameter, geodesics and distances, connectedness, eccentricity, clustering coefficient, walks, reciprocity etc.

- Matrix routines: Adjacency plot, Laplacian matrix, Degree matrix, Cocitation, etc.

- Advanced structural measures for social network analysis such as centrality and prestige indices (i.e. eigenvector and closeness centrality, betweenness centrality, information centrality, power centrality, proximity and pagerank prestige), 

- Community detection algorithms such as triad census, clique census, etc.

- Structural equivalence analysis, using hierarchical clustering, actor similarities and tie profile dissimilarities, pearson coefficients, etc.

- Random network creation, i.e. Erdos-Renyi, Watts-Strogatz, scale-free, lattice, etc.

- One-click recreation of well-known social network datasets such as Padgett's Florentine families.

- Layout algorithms based on either prominence indices (i.e. circular, level and nodal sizes by centrality score) or force-directed models (i.e. Kamada-Kawai, Fruchterman-Reingold, etc) for meaningful visualizations of your social network data.

- Multirelational loading and editing. You can load a network consisting of multiple relations or create a network on your own and add multiple relations to it.

- Built-in web crawler allowing you to automatically create networks from links found in a given initial URL.

- Comprehensive documentation both online and while running the application explaining each feature and algorithm of SocNetV in detail.

- Binary packages and installers for Windows, Linux and macOS.

## 3. Availability & License

Official Website: <https://socnetv.org>

Email: <info@socnetv.org>

Author: Dimitris V. Kalamaras

Blog:   <https://dimitris.apeiro.gr>

SocNetV is a cross-platform application developed in C++ and Qt, an open source software development platform published under the GPL.

This means you can compile and run SocNetV on any Operating System supported by Qt.
See available packages and installation instructions below.

SocNetV is Free Software, distributed under the General Public Licence Version 3 (see the COPYING file for details).
The documentation is also Free, licensed under the Free Documentation License (FDL).

The application is not a "finished" product. Therefore, there is no warranty of efficiency, correctness or usability.

Nevertheless, we are looking forward to help you if you experience any problems with SocNetV! See bug reporting below.

## 4. Installation

SocNetV is multi-platform, which means that it can be installed and run in every Operating System supported by the Qt toolkit.

The project offers binaries and installers for the three major Operating Systems:
Windows, macOS and Linux.

If there is no binary package for your OS, please download and compile the source code,
as explained further below.

### a. Install a binary package or installer (Linux/macOS/Windows)

You can download an installer or a binary package for your Operating System from the
project's Downloads page: <https://socnetv.org/downloads>

Follow the instructions below to install it in your system.

#### Install in Windows

To install SocNetV in Windows, download the latest SocNetV Windows installer from
the [Downloads](https://socnetv.org/downloads) page, and double-click on the executable to start the installation.

Note: You might see a Windows pop up about unknown software origin/publisher. Please ignore it and proceed, as we do not sign our released packages with any code signing certificates (which are not free by the way). 

Click Next and Accept the License (GPL) to install the program.

The program will be installed in the usual Windows Program Files directory and a new
Start Menu shortcut will be created.

Afterwards you can run the application from your Start menu.

#### Install in macOS

To install SocNetV in macOS, download the latest SocNetV macOS package from
the [Downloads](https://socnetv.org/downloads) page. Then right-click on it and select Open.

If the package is an installer, the installation will start immediately and the application 
will be installed automatically in your Applications.

Otherwise, if the package is just an macOS image disk (a file with a .dmg extension), then double-click on it to open it. 
You will see a new window with the SocNetV executable icon inside. Right-click on it and select Open to run the application. 

Note: The first time you will run SocNetV, macOS may tell you that it cannot verify the software developer/publisher. That's because we do not sign our released packages with any code signing certificates (which are not free by the way). 
Please press Cancel, not Move to Bin! Then, right-click again on SocNetV app and select Open to run the application normally.  

After that, in order to permanently install SocNetV in your macOS, you can simply drag the SocNetV executable icon into your Applications.

Alternatively, there is a SocNetV port in MacPorts (thanks to Szabolcs Horvát!). It can be installed with `port install socnetv`.

#### Install in Linux

To run the latest and greatest version of SocNetV in Linux, download the latest Linux AppImage from
the project's [Downloads](https://socnetv.org/downloads) page.

Then, make the .AppImage file executable and double-click on it to run SocNetV. That's it! Note, however, that your system needs to have libfuse2 installed. On latest Ubuntu releases, you need to instal it with `sudo apt install libfuse2t64`

> SocNetV is also available in the [repositories of most Linux distributions](https://repology.org/project/socnetv/versions). 
However, that is not always the most recent version. We urge you to use the AppImage of the latest version available from our website instead.

Alternatively, users of openSUSE, Fedora and Ubuntu/Debian can install SocNetV from our own repositories.

For Debian and Ubuntu, use the following commands to add our repository and install SocNetV:

```bash
sudo add-apt-repository ppa:dimitris-kalamaras/ppa
sudo apt-get update
sudo apt-get install socnetv
```

In Fedora and openSUSE, choose and add the correct repository for your distro version 
from here: https://software.opensuse.org/download.html?project=home%3Aoxy86&package=socnetv 

Once you add the repo, install SocNetV using the command (Fedora):

```bash
sudo yum install socnetv
```

or (openSUSE):

```bash
sudo zypper in socnetv
```

### b. Compile from Source Code

To compile and install SocNetV from source you need the Qt toolkit
development libraries, version 6.

Qt is an open source C++ toolkit, for Windows, Linux and MacOS.

Windows and MacOS users should download and install Qt6 from <https://www.qt.io/developers>

Linux users need to install the following packages:

openSUSE: libqt6-qtbase, libqt6-qtbase-devel, libQt6Charts6-devel, qt6-tools

Fedora: qt6-qtbase, qt6-qtbase-devel, qt6-qtcharts-devel, qt6-linguist, qt6-qt5compat

Debian: qt6-base-dev, qt6-base-dev-tools, qt6-charts-dev, qt6-svg-dev, qt6-5compat-dev, libqt6opengl6-dev

Once you have Qt installed, you are ready to compile SocNetV from source.

Download the archive with the source code of the latest version from
<https://github.com/socnetv/app/releases/latest>. You will get a compressed file like app-3.0.tar.gz

Then type in the following commands in order to decompress the
SocNetV source code, configure and build it into a executable binary. The commands require that the command `qmake` is in your PATH, otherwise you will not be able to configure the build and it will fail. Replace 3.X with the version you downloaded.

```bash
untar zxfv app-3.X.tar.gz
cd app-3.X
qmake
make
sudo make install # or su -c 'make install'
```

Probably you have already done the first 2 steps, so just type in 'qmake' or 'qmake6'.
When you finish compiling and installing, run the application typing:

```bash
socnetv
```

or go to Start Menu > Mathematics  > SocNetV.

Alternatively, if you have `cmake` installed, you can use it like this:

```bash
untar zxfv app-3.X.tar.gz
cd app-3.X
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/ver/gcc_64/lib/cmake 
cmake --build build
```

The executable `socnetv` will be placed inside the `build` folder.

## 5. Command Line Options

SocNetV is primarily a GUI program. Nevertheless, some command line options are available:

```bash
Options:
  -h, --help           Displays this help.
  -v, --version        Displays version information.
  -p, --progress       Force showing progress dialogs/bars during computations.
  --nm, --notmaximized Do not show the app maximized.
  -f, --fullscreen     Show in full screen mode.
  -d, --debug <level>  Print debug messages to stdout/console. Available
                       verbosity <level>s: 'none', 'min' or 'full'. Default:
                       'min'.

Arguments:
  file                 Network file to load on startup. You can load a network
                       from a file using `socnetv file.net` where
                       file.net/csv/dot/graphml must be of valid format. See
                       README.
```

For example, type:

```bash
./socnetv net.graphml
```

to start SocNetV and immediately load network file named 'net.graphml' (in current folder).

## 6. Usage & documentation

To help you work with the application, there are tooltips and What's This help messages
inside the application, when running SocNetV.

To see the full documentation, press F1. It will open a browser window with the SocNetV Manual,
which is available online at the project's website: <https://socnetv.org/documentation/>

For a full list of changes, see our CHANGELOG.md

## 7. Bug reporting & contact

If you have a bug report or a feature request, please file it in our GitHub issue tracker:
https://github.com/socnetv/app/issues

To contact us directly, send an email to: <info@socnetv.org>

