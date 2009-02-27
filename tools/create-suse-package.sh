#!/bin/bash

#CHANGE THIS TO NEW VERSION NUMBERS
VER=0.52;
echo $VER

echo .
echo ---------------------------------
echo   openSUSE RPM Package Creator
echo    Author: Dimitris Kalamaras
echo ---------------------------------

cd ~/Documents/socnetv/trunk

if [ -d ../opensuse ];    then
        echo Removing old opensuse directory
	rm -rf ../opensuse
else
        echo No older opensuse directory. Continuing...
      
fi


echo
echo Checking if RPM development tree exists...
if [ -d /usr/src/packages/RPMS ];    then
        echo RPM development tree exists. Continuing.
else
        echo Seems there is no RPM development tree. Enter sudo password to create it
        sudo mkdir /usr/src/packages/BUILD  /usr/src/packages/RPMS  /usr/src/packages/SOURCES  /usr/src/packages/SPECS  /usr/src/packages/SRPMS
        echo  OK;
fi



oldrpm=`find /usr/src/packages/RPMS -type f -name *.rpm`;
for file in $oldrpm; do
        if [ -f "$file" ];  then
		echo old RPM $file;
		echo Seems there are old rpms in /usr/src/packages/RPMS/... Enter password to remove them
                sudo rm $file
        else
                echo No older RPM packages.Continuing....
                exit;
        fi
done


oldbz2=`find /usr/src/packages/SOURCES -type f -name *.bz2`;
for file in $oldbz2; do
        if [ -f "$file" ];  then
                echo old bz2 $file;
                echo Seems there are old rpms in /usr/src/packages/SOURCES/... Enter password to remove them
                sudo rm $file
        else
                echo No older sources packages.Continuing....
                exit;
        fi
done




echo .
echo ---------------------------------
echo    CLEANING UP COMPILED FILES   
echo ---------------------------------

./configure > /dev/null 2>&1
make clean
if [ -f socnetv ];    then
        echo Removing old SocNetV binary
	rm socnetv 
else
        echo No older binary. Continuing....
fi



oldfiles=`find . -type f -name *~`;
for i in $oldfiles; do
        echo Removing $i;
        rm $i;
done;



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

sudo rpmbuild -ba socnetv.spec


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
ls -lsh

rpmlint *

echo .
echo ---------------------------------
echo      RPM PACKAGE READY     
echo ---------------------------------
echo .

ls -lsh

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








