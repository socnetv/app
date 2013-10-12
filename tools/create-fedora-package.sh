#!/bin/bash

#CHANGE THIS TO NEW VERSION NUMBERS
VER=0.80;
echo $VER

echo .
echo ---------------------------------
echo   Fedora RPM Package Creator
echo   Copyright Dimitris Kalamaras
echo   License: GNU GPL v3
echo ---------------------------------

echo Testing if rpmlint and rpmdevtools are installed...

if [ -f /usr/bin/rpmlint ] &&  [ -f /usr/bin/rpmdev-setuptree  ] && [ -f /usr/bin/autoconf ]; then
        echo OK. Necessary tools are installed;
else
        echo No rpmlint or rpmdevtools. Enter sudo password to install them: 
        su -c 'yum install rpmlint rpmdevtools  autoconf'
fi



echo
echo Checking if home RPM development tree exists...
if [ -d ~/rpmbuild/ ];    then
	echo Home RPM development tree exists. Recreating it....
	rm -rf ~/rpmbuild
	rpmdev-setuptree
else
        echo Seems there is no RPM development tree. Creating it...
        rpmdev-setuptree
        echo  OK;
fi




cd ~/socnetv/trunk


echo Checking for old working dir...
if [ -d ../fedora ];    then
        echo Removing old fedora working directory
        rm -rf ../fedora
else
        echo No older fedora directory. Continuing.
fi


echo .
echo ---------------------------------
echo    CLEANING UP COMPILED FILES   
echo ---------------------------------

autoconf

echo ""

./configure > /dev/null 2>&1

echo  ""

make clean

echo ""

echo "testing if a binary exists..."

if [ -f socnetv ]; then
	 rm socnetv 
else
	echo "No socnetv binary"
fi


echo removing old backup files
oldfiles=`find . -type f -name *~`;
for i in $oldfiles; do 
	echo Removing $i; 
	rm $i;
done;

chmod 644 nets/*
find . -type f -name '*~' -delete
rm -f config.log config.status Makefile socnetv.mak

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

echo .
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

echo Copying spec to ~/rpmbuild/SPECS/

cp socnetv.spec ~/rpmbuild/SPECS/
   

if [ -f ~/rpmbuild/SPECS/socnetv.spec ];    then
        echo spec file copied ok;
else
        echo Sorry. No spec file copied....
        exit;
fi


echo Copying bz2 to ~/rpmbuild/SOURCES/
cp ../*.bz2  ~/rpmbuild/SOURCES/

if [ -f ~/rpmbuild/SOURCES/SocNetV-$VER.tar.bz2 ];    then
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

ls -l ~/rpmbuild/RPMS/i386/*.rpm > /dev/null 2>&1

if [ "$?" = "0" ]; then 
	echo "file ok";
else 
	echo "Sorry. No RPM package..."
	exit
fi

echo .
echo ---------------------------------
echo        TESTING PACKAGE         
echo ---------------------------------
cd ~/rpmbuild/RPMS/i386
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








