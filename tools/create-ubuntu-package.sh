#!/bin/bash



#CHANGE THIS TO NEW VERSION NUMBERS
VER=0.80;   
echo $VER

echo .
echo ---------------------------------
echo    Ubuntu Deb Package Creator
echo    Copyright Dimitris Kalamaras
echo	License: GNU GPL v3
echo ---------------------------------
echo 



echo Testing if lintian and devscripts are installed...
ans=`dpkg-query -l 'lintian*' | grep lintian | awk '{ print $2 }'`

if [ $ans =  "lintian" ]; then 
	echo OK. Necessary tools are installed; 
else 
	echo No lintian or devscripts. Enter sudo password to install them: 
	sudo apt-get install devscripts pbuilder lintian dput debhelper
fi




cd ~/socnetv/trunk

echo Removing old ubuntu directory
if [ -d ../ubuntu ];    then
        echo Removing old ubuntu directory
	rm -rf ../ubuntu
else
	echo No older ubuntu directory. Continuing...
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

chmod 644 nets/*
find . -type f -name '*~' -delete
rm -f config.log config.status Makefile socnetv.spec socnetv.mak



echo Asking for changelog entry....

dch -i

echo .
echo ---------------------------------
echo   COPY FILES TO WORKING DIRS     
echo ---------------------------------


find . -not -name "qdevelop-*" -not -name "socnetv" -not -name ".qdevelop"  -not -name "pajek*" -not -path "*./autom4te.cache*" -not -path "*.svn*" -not -path "*./test-nets*"  -print0  | cpio -pmd0 ../ubuntu/socnetv-$VER

find . -not -name "qdevelop-*" -not -name "socnetv" -not -name ".qdevelop"  -not -name "pajek*" -not -path "*./autom4te.cache*" -not -path "*.svn*" -not -path "*./test-nets*"  -print0  | cpio -pmd0 ../ubuntu/socnetv-$VER.orig



echo .
echo ---------------------------------
echo    GOTO TO WORKING DIRECTORY    
echo ---------------------------------

cd ../ubuntu/
echo "Make tarballs? (Y/N)"
read ans
if [ $ans = "N" ]; then
        exit;
elif [ $ans = "n" ]; then    
        exit;
fi

tar zcfv SocNetV-$VER.tar.gz socnetv-$VER/
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
echo    START TEST PACKAGE CREATION       
echo ---------------------------------

debuild 


echo .
echo ---------------------------------
echo     SOURCE TEST PACKAGE CREATION     
echo ---------------------------------
echo .

debuild -S

echo .
echo ---------------------------------
echo        TESTING TEST PACKAGE         
echo ---------------------------------
cd ..

lintian -Ivi *.dsc


echo Have I build something?

ls *.deb -lh

echo .
if [ -f *.deb ];    then
        echo test package is ready!
else
	echo No DEB...
fi

echo "Proceed? (Y/N)"
read ans
if [ $ans = "N" ]; then
        exit;

elif [ $ans = "n" ]; then    
        exit;
fi




echo .
echo ---------------------------------
echo    START FINAL PACKAGE CREATION 
echo ---------------------------------
echo .

cd socnetv-$VER/
debuild -S -sa 

echo Check if final DEB has been created...
echo
cd ..
ls -lh *.deb 
echo
if [ -f *.deb ];    then
        echo "DEB package is ready! Installing it...";
	sudo dpkg -i *.deb
	if [ -f /usr/bin/socnetv ]; then 
		echo "Package installed OK"; 
	else 
		echo "Error Exiting"; 
		exit; 
	fi
else
	echo "No DEB! Exiting";
	exit;
fi



echo . 
echo "Upload .changes file to Launchpad PPA? (Y/N)";
read ans
if [ $ans = "N" ]; then
        exit;

elif [ $ans = "n" ]; then    
        exit;
fi

echo .
echo ---------------------------------
echo   	UPLOAD FINAL PACKAGE 
echo ---------------------------------
echo .

VER=`grep urgency socnetv-$VER/debian/changelog | awk '{ print $2 } ' | head -n 1 | sed s/"("// |  sed s/")"//`
echo New package version: $VER   
echo "Proceed? (Y/N)"
read ans
if [ $ans = "N" ]; then
        exit;

elif [ $ans = "n" ]; then    
        exit;
fi

echo Last exit!
read ans
dput ppa socnetv_"$VER"_source.changes



echo
echo "Upload package to Sourceforge also? (Y/N)"
read ans
if [ $ans = "N" ]; then
        exit;

elif [ $ans = "n" ]; then    
        exit;
fi

echo .
echo ---------------------------------
echo   UPLOADING FINAL DEB PACKAGE 
echo ---------------------------------
echo .

rsync -avP -e ssh ../ubuntu/*.deb  oxy86@frs.sourceforge.net:uploads/


echo --------------------------------
echo bye!








