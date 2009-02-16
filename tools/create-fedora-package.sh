#!/bin/bash

#CHANGE THIS TO NEW VERSION NUMBERS
VER=0.51;
echo $VER

echo .
echo ---------------------------------
echo   Fedora RPM Package Creator
echo    Author: Dimitris Kalamaras
echo ---------------------------------

echo Setting up rpmdev-tree

rpmdev-setuptree

cd ~/Documents/socnetv/trunk

if [ -d ../fedora ];    then
        echo Removing old fedoradirectory
        rm -rf ../fedora
else
        echo No older fedora directory. Continuing...

fi

if [ -f ~/rpmbuild/RPMS/*.rpm ];    then
        echo Seems there are old rpms in ../RPMS/i586/... Enter password to remove them:
        rm ~/rpmbuild/RPMS/*.rpm
        echo removed old rpm;
else
        echo No older rpms. Continuing....
fi


echo .
echo ---------------------------------
echo    CLEANING UP COMPILED FILES   
echo ---------------------------------

./configure > /dev/null 2>&1

make clean
rm socnetv 


echo .
echo ---------------------------------
echo   COPY FILES TO WORKING DIRS     
echo ---------------------------------

find . -not -name "qdevelop-*" -not -name "pajek*" -not -path "*./autom4te.cache*" -not -path "*.svn*" -not -path "*./test-nets*" -not -path "./debian*"  -print0  | cpio -pmd0 ../fedora/socnetv-$VER



echo .
echo ---------------------------------
echo    GOTO TO WORKING DIRECTORY    
echo ---------------------------------

cd ../fedora/
echo "Make tarballs? (Y/N)"
read ans
if [ $ans = "N" ]; then
        exit;

elif [ $ans = "n" ]; then    
        exit;
fi

tar jcfv SocNetV-$VER.tar.bz2 socnetv-$VER/



cd socnetv-$VER/


echo "Start package creation? (Y/N)"
read ans
if [ $ans = "N" ]; then
        exit;

elif [ $ans = "n" ]; then    
        exit;
fi

echo .
echo ---------------------------------
echo    START PACKAGE CREATION       
echo ---------------------------------

echo Enter password to copy spec to ~/rpmbuild/SPECS/
cp socnetv.spec ~/rpmbuild/SPECS/
   

if [ -f ~/rpmbuild/SPECS/*.spec ];    then
        echo spec file copied ok;
else
        echo Sorry. No spec file copied....
        exit;
fi


echo Enter password to copy bz2 to ~/rpmbuild/SOURCES/
cp ../*.bz2  ~/rpmbuild/SOURCES/

if [ -f /rpmbuild/SOURCES/*.bz2 ];    then
        echo bz2 file copied ok;
else
        echo Sorry. No bz2 file copied....
        exit;
fi



cd ~/rpmbuild/SPECS/

echo .
echo ---------------------------------
echo     RPM PACKAGE CREATION     
echo ---------------------------------
echo .

echo Enter password to start package creation

rpmbuild -ba socnetv.spec


if [ -f ~/rpmbuild/RPMS/i586/*.rpm ];  	then
	echo file ok; 
else 
	echo Sorry. No RPM package....
	exit;
fi

echo .
echo ---------------------------------
echo        TESTING PACKAGE         
echo ---------------------------------
cd ~/rpmbuild/RPMS/i586
ls -ls

rpmlint *

echo .
echo ---------------------------------
echo      RPM PACKAGE READY     
echo ---------------------------------
echo .

ls -ls

echo "Upload package to Sourceforge? (Y/N)"
read ans
if [ $ans = "N" ]; then
        exit;

elif [ $ans = "n" ]; then    
        exit;
fi

echo .
echo ---------------------------------
echo   UPLOADING FINAL RPM PACKAGE 
echo ---------------------------------
echo .

rsync -avP -e ssh *.rpm  oxy86@frs.sourceforge.net:uploads/



echo --------------------------------
echo bye!








