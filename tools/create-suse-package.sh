#!/bin/bash

#CHANGE THIS TO NEW VERSION NUMBERS
VER=0.51;
echo $VER

echo .
echo ---------------------------------
echo   openSUSE RPM Package Creator
echo    Author: Dimitris Kalamaras
echo ---------------------------------

cd ~/Documents/socnetv/trunk

echo Removing old opensuse directory
rm -rf ../opensuse



echo .
echo ---------------------------------
echo    CLEANING UP COMPILED FILES   
echo ---------------------------------

make clean
rm socnetv 


echo .
echo ---------------------------------
echo   COPY FILES TO WORKING DIRS     
echo ---------------------------------

find . -not -name "qdevelop-*" -not -name "pajek*" -not -path "*./autom4te.cache*" -not -path "*.svn*" -not -path "*./test-nets*" -not -path "./debian*"  -print0  | cpio -pmd0 ../opensuse/socnetv-$VER



echo .
echo ---------------------------------
echo    GOTO TO WORKING DIRECTORY    
echo ---------------------------------

cd ../opensuse/
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

echo Enter password to copy spec to /usr/src/packages/SPECS/
su  -c 'cp socnetv.spec /usr/src/packages/SPECS/'
   

if [ -f /usr/src/packages/SPECS/*.spec ];    then
        echo spec file copied ok;
else
        echo Sorry. No spec file copied....
        exit;
fi


echo Enter password to copy bz2 to /usr/src/packages/SOURCES/
su -c 'cp ../*.bz2  /usr/src/packages/SOURCES/'

if [ -f /usr/src/packages/SOURCES/*.bz2 ];    then
        echo bz2 file copied ok;
else
        echo Sorry. No bz2 file copied....
        exit;
fi



cd /usr/src/packages/SPECS/

echo .
echo ---------------------------------
echo     RPM PACKAGE CREATION     
echo ---------------------------------
echo .

echo Enter password to start package creation

su -c  'rpmbuild -ba socnetv.spec'


if [ -f /usr/src/packages/RPMS/i586/*.rpm ];  	then
	echo file ok; 
else 
	echo Sorry. No RPM package....
	exit;
fi

echo .
echo ---------------------------------
echo        TESTING PACKAGE         
echo ---------------------------------
cd /usr/src/packages/RPMS/i586
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








