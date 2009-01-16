#!/bin/bash

cd ~/Documents/socnetv/trunk

VER=0.49;   
#CHANGE THIS TO NEW VERSION NUMBERS
echo $VER

echo .
echo ---------------------------------
echo !  CLEANING UP COMPILED FILES   !
echo ---------------------------------

make clean

echo .
echo ---------------------------------
echo !  COPY FILES TO WORKING DIRS   ! 
echo ---------------------------------

find . -not -path *.svn* -not -path *test-nets* -print0  | cpio -pmd0 ../socnetv/socnetv-$VER
find . -not -path *.svn* -not -path *test-nets* -print0  | cpio -pmd0 ../socnetv/socnetv-$VER.orig


echo .
echo ---------------------------------
echo !  GOTO TO WORKING DIRECTORY    !
echo ---------------------------------

cd ../socnetv/
echo "Make tarballs? (Y/N)"
read ans
if [ $ans = "N" ]; then
        exit;

elif [ $ans = "n" ]; then    
        exit;
fi

tar zcfv SocNetV-$VER.tar.gz socnetv-$VER/
tar jcfv SocNetV-$VER.tar.bz2 socnetv-$VER/



echo "Upload tarballs to Sourceforge also? (Y/N)"
read ans
if [ $ans = "N" ]; then
        exit;

elif [ $ans = "n" ]; then    
        exit;
fi

echo ---------------------------------
echo ! UPLOAD TO SOURCEFORGE         !
echo ---------------------------------
echo .

rsync -avP -e ssh SocNetV-$VER.tar.gz  oxy86@frs.sourceforge.net:uploads/
rsync -avP -e ssh SocNetV-$VER.tar.bz2  oxy86@frs.sourceforge.net:uploads/
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
echo !  START PACKAGE CREATION       !
echo ---------------------------------

debuild 


echo .
echo ---------------------------------
echo !   SOURCE PACKAGE CREATION     |
echo ---------------------------------
echo .
debuild -S

echo .
echo ---------------------------------
echo !       TESTING PACKAGE         !
echo ---------------------------------
cd ..
lintian -Ivi *.dsc

echo .
echo ---------------------------------
echo !    INITIAL PACKAGES READY     !
echo ---------------------------------
echo .
ls

echo .
echo ---------------------------------
echo !  START FINAL PACKAGE CREATION !
echo ---------------------------------
echo .
cd socnetv-$VER/
debuild -S -sa 

echo .
cd ..
ls 


echo "Upload package to Launchpad PPA? (Y/N)"
read ans
if [ $ans = "N" ]; then
        exit;

elif [ $ans = "n" ]; then    
        exit;
fi

echo .
echo ---------------------------------
echo ! UPLOAD FINAL PACKAGE CREATION !
echo ---------------------------------
echo .
echo "enter version number, i.e. 0.49-2"
read VER
dput ppa socnetv_$VER_source.changes



echo --------------------------------
echo bye!








