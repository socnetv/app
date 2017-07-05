[![socnetv](/src/images/socnetv.png)](http://socnetv.org) SocNetV - Social Network Visualizer
====================================



1. Overview
------------

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

- Random network creation, i.e. Erdos-Renyi, Watts-Strogatz, scale-free, ring lattice, etc.

- One-click recreation of well-known social network datasets such as Padgett's Florentine families.

- Layout algorithms based on either prominence indices (i.e. circular, level and nodal sizes 
by centrality score) or force-directed models (i.e. Eades, Fruchterman-Reingold, etc) 
for meaningful visualizations of your social network data.

- Multirelational loading and editing. You can load a network consisting of multiple 
relations or create a network on your own and add multiple relations to it. 

- Built-in web crawler, allowing you to automatically create networks from links found in a given initial URL.

- Comprehensive documentation, both online and while running the 
application, which explains each feature and algorithm of SocNetV in detail.

- Binary packages for Windows, Linux and MacOS.

The program is Free Software, licensed under the GNU General Public License 3 (GPL3).
You can copy it as many times as you wish, or modify it, provided you keep the 
same license. 

The documentation is also Free, licensed under the Free Documentation License (FDL).


2. Availability & License
-------------------------

Official Website: http://socnetv.org
Email: info@socnetv.org

Author: Dimitris V. Kalamaras 
Blog:   http://dimitris.apeiro.gr

SocNetV is a cross-platform application, developed in C++ language 
using the Qt5 cross-platform libraries and tools. Qt is an open source 
development toolkit published under the GPL.

This means you can compile and run SocNetV on any OS supported by Qt. 
Binary packages and executables for Linux, Mac and Windows are available from 
the project's website. See section 3 (installation) below.

SocNetV is Free Software, distributed under the General Public Licence Version 3 
(see the COPYING file for details). 

The application is not a "finished" product. Therefore, there is no warranty of 
efficiency, correctness or usability. 

Nevertheless, we are looking forward to help you if you experience any problems 
with SocNetV. See section 6 (bug reporting) below.



3. Installation
---------------

You can install SocNetV :

- using binary packages or
- compiling it from source 


## a) Install a binary package or executable (Linux/Mac/Windows)

SocNetV binary packages are available for Windows, MacOS and Linux.

You can download a binary package for your Operating System from the project's
webpage at: http://socnetv.org/downloads

If there is no package for your OS, please download and compile the source code.

Windows

To run SocNetV in Windows, download the latest SocNetV Windows installer from the 
project's Downloads page and double-click on the executable to start the installation.
The program will be installed in the usual Windows Program Files directory and a new 
Start Menu shortcut will be created. Click on that shortcut to start SocNetV.

MacOS

If you are a Mac user, download the latest SocNetV disk image (.dmg file) from the 
Downloads page. 
Once downloaded, double click on the .dmg file and a new window will appear. 
Drag the SocNetV icon into your Applications folder to install it.
To run the application, double click on the SocNetV icon in Applications holding 
down the META key.

Linux

SocNetV packages are available in the repositories of most Linux distributions, although 
these are usually not the latest version. 

To install the latest and greatest SocNetV version, users of openSUSE, Fedora and 
Ubuntu/Debian are advised to add our own repositories to their systems.

In Debian and Ubuntu, add our repository and install SocNetV with these commands:

sudo add-apt-repository ppa:dimitris-kalamaras/ppa
sudo apt-get update
sudo apt-get install socnetv

In Fedora and openSUSE, choose and add the correct repository for your distro version 
from here: 
http://download.opensuse.org/repositories/home:/oxy86/ 

Once you add the repo, install SocNetV using the command (Fedora):
sudo yum install socnetv

or (openSUSE): 
sudo zypper in socnetv



## b) Compile from Source Code

To compile and install SocNetV from source you need the Qt5 toolkit development 
libraries. Qt5 is preinstalled in most Linux distributions and it is also available 
for Windows and MacOS.  If you do not have Qt5 installed, please download and
install it from https://www.qt.io/developers

Once you have Qt5 installed in your OS, you are ready to compile SocNetV from source.
Download the archive with the source code of the latest version from 
https://github.com/socnetv/app/releases/latest, i.e. SocNetV-2.x.tar.gz

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

