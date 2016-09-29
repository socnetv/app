SocNetV -A Social Network Visualizer
====================================


\tableofcontents

1. Overview
------------

Social Network Visualizer (SocNetV) is a cross-platform, user-friendly and free
software application for social network analysis and visualization.

Using SocNetV you draw social networks (mathematical graphs) with a few clicks 
on a virtual canvas or load social network data of various formats (GraphML, 
GraphViz, Adjacency, Pajek, UCINET, etc).

SocNetV enables you to modify the social networks, analyse their social and 
mathematical properties, produce reports for these properties and apply 
visualization layouts for relevant presentation of each network. 

The application supports multirelational loading and editing. 
You can load a network consisting of multiple relations or create a network 
on your own and add multiple relations to it.

SocNetV computes graph-theoretic properties, such as density, diameter, geodesics 
and distances (geodesic lengths), connectedness, eccentricity, etc. 
It also calculates advanced structural measures for social network analysis 
such as centrality and prestige indices (i.e. closeness centrality, betweeness 
centrality, information centrality, power centrality, proximity and rank prestige), 
triad census, cliques, clustering coefficient, etc.

Furthermore, random networks (Erdos-Renyi, Watts-Strogatz, ring lattice, etc) and 
well-known social network datasets (i.e. Padgett's Florentine families) can be easily 
recreated. 

SocNetV also offers a built-in web crawler, allowing you to automatically create 
networks from links found in a given initial URL.

The application supports various layout algorithms based on either prominence indices 
(i.e. circular, level and nodal sizes by centrality score) or force-directed models 
(i.e. Eades, Fruchterman-Reingold, etc) for meaningful visualizations of your social 
network data.

There is also comprehensive documentation, both online and while running the 
application, which explains each feature and algorithm of SocNetV in detail.

SocNetV runs in Windows, Linux and Mac OS X.

The program is Free Software, licensed under the GNU General Public License 3 (GPL3).
You can copy it as many times as you wish, or modify it, provided you keep the 
same license. 

The documentation is also Free, licensed under the Free Documentation License (FDL).


2. Availability & License
-------------------------

Official Website: http://socnetv.sourceforge.net

Author: Dimitris V. Kalamaras <dimitris.kalamaras@gmail.com>
Blog:   http://dimitris.apeiro.gr

SocNetV is a cross-platform application, developed in C++ language 
using the Qt5 cross-platform libraries and tools.

This means you can compile and run SocNetV on Linux, Mac and Windows. 

SocNetV is Free Software, distributed under the General Public Licence Version 3 
(see the COPYING file for details). 

The application is not a "finished" product. Therefore, 
there is no warranty of efficiency, correctness or usability. 
Nevertheless, we are looking forward to help you if you have any problem. 
See section 6 (bug reporting) below.



3. Installation
---------------

You can install SocNetV :

b) using binary packages.
a) compiling it from source or


## a) Install a binary package or executable (Linux/Mac/Windows)

SocNetV binary packages are available for Windows, Mac OS X and Linux distros.

You can download a binary package for your Operating System from the project
webpage at: http://socnetv.sourceforge.net/downloads

If there is no package for your OS, please download and compile the source code.

Windows

To run SocNetV in Windows, download the latest SocNetV Windows installer from the 
project's Downloads page and double-click on the executable to start the installation.
The program will be installed in the usual Windows Program Files directory and a new 
Start Menu shortcut will be created. Click on that shortcut to start SocNetV immediately.

Mac OS X

If you are a Mac user, you can download a SocNetV disk image (dmg file) to install it. 
From the Downloads page, download the Mac OS .dmg file. 
Once downloaded, double click on it and a new window will appear. Drag the SocNetV icon
into your Applications folder to install it.
To run the application, double click on the SocNetV icon holding down the META key.

Linux

SocNetV is available in most Linux distributions, although not the latest version. 

To install the latest and greatest SocNetV version, users of openSUSE, Fedora and 
Ubuntu/Debian are advised to add our own repositories to their systems.

In Debian and Ubuntu, install SocNetV from our repos with these commands:

sudo add-apt-repository ppa:dimitris-kalamaras/ppa
sudo apt-get update
sudo apt-get install socnetv

In Fedora and openSUSE, choose and add the correct repository from here: 
http://download.opensuse.org/repositories/home:/oxy86/ 

Once you add the repo, install SocNetV using the command (Fedora):
sudo yum install socnetv

or (openSUSE): 
sudo zypper in socnetv



## b) Compile from Source Code

To compile and install SocNetV from source you need the Qt5 toolkit development 
libraries. Qt is an open source C++ toolkit published under the GPL. 
Qt5 is preinstalled in most Linux distributions and it is available for
Windows and Mac OS X.  If you do not have Qt5 installed, please download and
install it from https://www.qt.io/developers

Once you have Qt5 installed in your OS, you are ready to compile SocNetV from source.
Download the tarball archive with the source code of the latest SocNetV version 
(you probably already have this :P). 

All you have to do is to type in the following commands in order to decompress the
SocNetV tarball and build it. Replace 2.X with the version you downloaded.

1) untar zxfv SocNetV-2.X.tar.gz

2) cd socnetv-2.X

3) qmake

4) make

5) sudo make install or su -c 'make install'

Probably you have already done the first 2 steps, so just type in 'qmake' or 'qmake-qt5'.

When you finish compiling and installing, run the application typing:

socnetv

or go to Start Menu > Mathematics  > SocNetV.



# 4. Command Line Options
	
SocNetV is primarily a GUI program. Nevertheless, some command line options 
are available. Type:

./socnetv filename.net

to start socnetv with network named filename.net loaded.



# 5. Usage 

For usage documentation, see online help.

Or, when running SocNetV, press F1 to display the SocNetV Manual.
The manual is also available at the project's website.


# 6. Bug reporting

Please, file any bug reports in our bug tracker:
https://bugs.launchpad.net/socnetv/+filebug


