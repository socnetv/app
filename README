SocNetV -A Social Network Visualizer
-------------------------------------

1. Overview
-----------

Social Network Visualizer (SocNetV) is a cross-platform, user-friendly tool 
for the analysis and visualization of Social Networks. 

It lets you construct social networks (mathematical graphs) with a few clicks 
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

SocNetV offers various layout algorithms based on either prominence indices 
(i.e. circular, level and nodal sizes by centrality score) or force-directed 
models (i.e. Eades, Fruchterman-Reingold, etc) for meaningful visualizations 
of the social networks.

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
My Blog:   http://dimitris.apeiro.gr

SocNetV is a cross-platform application, developed in C++ language 
using the Qt5 multiplatform library and tools.

This means you can compile and run SocNetV on Linux, Mac and Windows. 

SocNetV is Free Software, distributed under the General Public Licence Version 3 
(see the COPYING file for details). 

The application is not a "finished" product. Therefore, 
there is no warranty of efficiency, correctness or usability. 
Nevertheless, we are looking forward to help you if you have any problem. 
See section 6 (bug reporting) below.



3. Installation
---------------

You can install SocNetV by:

a) compiling it from source or 
b) using binary packages.

In either case, you need Qt 5 for versions 1.x 
Most Linux Distros have Qt installed by default.
Windows and OS X users please go to http://qt-project.org to download Qt5 library.
 

If you cannot install Qt5 you can try the 0.x series of SocNetV which work with Qt4. 
Please note that SocNetV uses QtWebKit to display online help. 
QtWebKit has been added to Qt from version 4.4, which means you can't compile 
SocNetV in distros with older releases of Qt.

a) Compile from Source Code
	
To compile from source code, download the tarball archive with the source code 
of the latest SocNetV version (you probably already have this :P). 
Then, untar (decompress) the archive using a command like this:

tar zxfv SocNetV-1.X.tar.gz

Then enter the new directory and compile with these commands:

cd socnetv-1.XX
qmake (or qmake-qt5)
make

Now you can install it using:

su -c 'make install' 

or 

sudo make install 

If everything is ok, then you can run SocNetV by entering:

socnetv


b) Install a binary package or executable (Linux/Mac/Windows)

To install SocNetV from a binary package for Linux or an executable for 
Windows, check http://socnetv.sourceforge.net/downloads.html and see if 
there is a package of the latest version for your operating system. 

Please note that SocNetV is also available in most Linux distributions,
although not always the latest version.

In Debian and Ubuntu, install SocNetV from repos with:

sudo apt-get install socnetv

In Fedora, use the command:
sudo yum install socnetv

In openSUSE:
sudo zypper in socnetv

Mac OS users may download the disk image of the latest version from 
http://socnetv.sourceforge.net/downloads.html.

Double click on the .img file, then on the new window click socnetv icon 
while pressing down the meta key.

You can also find versions for Mac on the Internet, although these are 
not supported. See: http://pdb.finkproject.org/pdb/package.php/socnetv-mac


4. Command Line Options
-----------------------
	
SocNetV is primarily a GUI program. Nevertheless, some command line options 
are available. Type:

1) ./socnetv filename.net
   to start snv with network named filename.net loaded.
2) ./socnetv -v
   to print version of snv and exit.
3) ./socnetv -d 
   to enable debugging mode, in which snv prints comprehensive messages about 
   what it is doing.



5. Usage 
--------

For usage documentation, see online help.

Or, when running SocNetV, press F1 to display the SocNetV Manual.

There are some example networks inside the /usr/local/doc/socnetv/net folder.
Just press Ctrl+O, go there  and choose one file.


6. Bug reporting
----------------

Please, file any bug reports in our bug tracker:
https://bugs.launchpad.net/socnetv/+filebug


7. Note to packagers
--------------------

Packagers: please note that the SocNetV manual is copied to 
$(DESTDIR)$(prefix)/doc/$(name)/manual
