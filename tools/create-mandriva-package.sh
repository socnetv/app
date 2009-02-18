#!/bin/bash

#CHANGE THIS TO NEW VERSION NUMBERS
VER=0.51;
echo $VER

echo .
echo ---------------------------------
echo   Mandriva RPM Package Creator
echo    Copyright Dimitris Kalamaras
echo	License: GNU GPL v3
echo ---------------------------------
echo 

echo Testing if rpmlint and rpmdevtools are installed...
ans=`urpmq  rpmdevtools rpmlint | grep rpmlint ` 
if [ $ans =  "rpmlint" ]; then 
	echo OK. Necessary tools are installed; 
else 
	echo No rpmlint or rpmdevtools. Enter sudo password to install them: 
	sudo urpmi rpmlint rpmdevtools
fi


echo
echo Checking for old working dir...
cd ~/Documents/socnetv/trunk

if [ -d ../mandriva ];    then
        echo Removing old mandriva directory
	rm -rf ../mandriva
else
        echo No older mandriva directory. Continuing...
      
fi

echo
echo Checking for old RPMs...
if [ -f /usr/src/rpm/RPMS/i586/*.rpm ];    then
	echo Seems there are old rpms in /usr/src/packages/RPMS/i586/... Enter sudo password to remove them:
	sudo rm /usr/src/rpm/RPMS/i586/*.rpm
        echo removed old rpm;
else
        echo No older rpms. Continuing....
fi

echo
echo Checking if RPM development tree exists...
if [ -d /usr/src/rpm/RPMS ];    then
	echo RPM development tree exists. Continuing.
else
        echo Seems there is no RPM development tree. Enter sudo password to create it
        sudo rpmdev-setuptree
        echo  OK;
fi


echo .
echo ---------------------------------
echo    CLEANING UP COMPILED FILES   
echo ---------------------------------

./configure > /dev/null 2>&1
make clean
rm socnetv 


oldfiles=`find . -type f -name *~`;
for i in $oldfiles; do 
	echo Removing $i; 
	rm $i;
done;

echo .

echo ---------------------------------
echo   COPY FILES TO WORKING DIRS     
echo ---------------------------------

find . -not -name "qdevelop-*" -not -name "pajek*" -not -path "*./autom4te.cache*" -not -path "*.svn*" -not -path "*./test-nets*" -not -path "./debian*"  -print0  | cpio -pmd0 ../mandriva/socnetv-$VER



echo .
echo ---------------------------------
echo    GOTO TO WORKING DIRECTORY    
echo ---------------------------------

cd ../mandriva/
echo "Make tarballs? (Y/N)"
read ans
if [ $ans = "N" ]; then
        exit;

elif [ $ans = "n" ]; then    
        exit;
fi

tar jcfv SocNetV-$VER.tar.bz2 socnetv-$VER/

cd socnetv-$VER/

echo
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

echo Enter password to copy spec to /usr/src/rpm/SPECS/
sudo cp socnetv.spec /usr/src/rpm/SPECS/
   

if [ -f /usr/src/rpm/SPECS/*.spec ];    then
        echo spec file copied ok;
else
        echo Sorry. No spec file copied....
        exit;
fi


echo Enter password to copy bz2 to /usr/src/rpm/SOURCES/
sudo cp ../*.bz2  /usr/src/rpm/SOURCES/

if [ -f /usr/src/rpm/SOURCES/*.bz2 ];    then
        echo bz2 file copied ok;
else
        echo Sorry. No bz2 file copied....
        exit;
fi


cd /usr/src/rpm/SPECS/


echo .
echo ---------------------------------
echo     RPM PACKAGE CREATION     
echo ---------------------------------
echo .

echo Enter sudo password to start package creation

sudo rpmbuild -ba socnetv.spec

newrpm=`find /usr/src/rpm/RPMS -type f -name *.rpm`;
for file in $newrpm; do
	if [ -f "$file" ];  then
		echo RPM $file .... OK; 
	else 
		echo Sorry. No RPM package....
		exit;
	fi
done
echo .
echo ---------------------------------
echo        TESTING PACKAGE         
echo ---------------------------------

cd /usr/src/rpm/RPMS/i586
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








