#!/bin/bash

cd ~/Documents/Code/Qt/socnetv/trunk
if [ -d "../release" ]; then
	echo .
	echo ---------------------------------
	echo      REMOVING OLD RELEASE   
	echo ---------------------------------
	rm -rf ../release;
fi

#CHANGE THIS TO NEW VERSION NUMBERS
VER=0.6.0;   
echo $VER

echo .
echo ---------------------------------
echo !  CLEANING UP COMPILED FILES   !
echo ---------------------------------

make clean
rm socnetv

echo .
echo ---------------------------------
echo 	     RUNNING AUTOCONF
echo --------------------------------
autoconf

echo .
echo ---------------------------------
echo !  COPY FILES TO WORKING DIRS   ! 
echo ---------------------------------

find . -not -name "qdevelop-*" -not -name "socnetv" -not -name ".qdevelop" -not -name "pajek*" -not -name "*.dat" -not -path "*./autom4te.cache*" -not -path "*.svn*" -not -path "*./test-nets*" -not -path "./debian*"  -print0  | cpio -pmd0 ../release/socnetv-$VER



echo .
echo ---------------------------------
echo !  GOTO TO WORKING DIRECTORY    !
echo ---------------------------------

cd ../release/
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


