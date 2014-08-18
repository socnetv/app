#!/bin/bash

cd ~/socnetv/trunk
if [ -d "../release" ]; then
	echo .
	echo ---------------------------------
	echo      REMOVING OLD RELEASE   
	echo ---------------------------------
	rm -rf ../release;
fi

#CHANGE THIS TO NEW VERSION NUMBERS
VER=1.0;   
echo "enter ver";
read VER
echo $VER

echo .
echo ---------------------------------
echo !  CLEANING UP COMPILED FILES   !
echo ---------------------------------

make clean
rm *.o
rm socnetv

echo .
echo ---------------------------------
echo !  COPY FILES TO WORKING DIRS   ! 
echo ---------------------------------

chmod 644 nets/*
find . -type f -name '*~' -delete
find . -type f -name '*.dat' -delete
rm -f *.log; ls *.in | sed 's/\.in/ /g' | xargs rm -f
rm -f config.log config.status Makefile  socnetv.mak 
find . -not -name "qdevelop-*"  -not -name "*.log" -not -name "socnetv" -not -name "*.o" -not -name "*.sm" -not -name "*.net" -not -name "*.graphml" -not -name "*.user" -not -name "socnetv.pro.user*" -not -name ".qdevelop" -not -name "pajek*"  -not -name "*.bak" -not -name "*.dat" -not -path "*./autom4te.cache*" -not -path "*.svn*" -not -path "*./test-nets*" -not -path "./debian*"  -print0  | cpio -pmd0 ../release/socnetv-$VER



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
ls socnetv-$VER/*.in | sed 's/\.in/ /g' | xargs rm -f
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

scp SocNetV-$VER.tar.gz  oxy86@frs.sourceforge.net:/home/frs/project/socnetv/$VER/
scp SocNetV-$VER.tar.bz2  oxy86@frs.sourceforge.net:/home/frs/project/socnetv/$VER/
cd socnetv-$VER/


