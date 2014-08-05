#!/bin/bash
echo "*******************";
echo "* Version updater *";
echo "*******************";

if [ -d /etc/mach_init.d ]; then
 workdir=/Users/dimitris/socnetv/trunk
else
 workdir=/home/dimitris/socnetv/trunk
fi
echo "using $workdir";
echo "Is this correct ? (Y/N)";
read ans;

if [ $ans = "N" ]
then
        echo "enter workdir: ";
	read workdir;
elif [ $ans = "n" ]
then
        echo "enter workdir: ";
        read workdir;
fi
cd $workdir

year=`date "+%Y"`;
echo "The year is now $year";
let lastyear=$year-1;
echo "Last year was $lastyear";
echo "Correct? (Y/N)"
if [ $ans = "N" ]
then
        echo "enter lastyear: ";
        read lastyear;
elif [ $ans = "n" ]
then
        echo "enter lastyear: ";
        read lastyear;
fi



update_version() {
	echo "Current version seems to be ....";
	grep "VERSION=" src/mainwindow.h

	echo "";
	echo "Enter old version:";
	read oldver
	echo "Enter new version:";
	read newver

	old="version: $oldver";
	new="version: $newver";

	echo "";
	echo "Changing headers and sources from "
	echo $old
	echo " to ....";
	echo $new
	perl -w -p -i.bak -e s/"$old"/"$new"/g src/*.cpp 
	perl -p -i.bak -e s/"$old"/"$new"/g src/*.h
	

	echo "";
	old="VERSION=\"$oldver\"";
	new="VERSION=\"$newver\"";
	echo "Changing VERSION tag to ....";
	echo $new
	perl -p -i.bak -e s/"$old"/"$new"/g src/*.cpp
	perl -p -i.bak -e s/"$old"/"$new"/g src/*.h
	
	echo "";
	old="zer version $oldver";
	new="zer version $newver";
	echo "Changing version from about screens to ....";
	echo $new
	perl -p -i.bak -e s/"$old"/"$new"/g src/*.cpp
	perl -p -i.bak -e s/"$old"/"$new"/g src/*.h
	
        old="2005-$lastyear";
        new="2005-$year";

        echo "";
        echo "Changing copyright year in headers and sources from "
        echo $old
        echo " to ....";
        echo $new
        perl -w -p -i.bak -e s/"$old"/"$new"/g src/*.cpp
        perl -p -i.bak -e s/"$old"/"$new"/g src/*.h


	echo "";
	old="SocNetV-$oldver";
	new="SocNetV-$newver";
	echo "Updating man page";
	echo $new
	gunzip man/socnetv.1.gz
	perl -p -i.bak -e s/"$old"/"$new"/g man/socnetv.*
	gzip man/socnetv.1

	echo "";
	old="v. $oldver"
	new="v. $newver"
	echo "Updating manual footer";
	perl -p -i.bak -e s/"$old"/"$new"/g manual/footer.html

	echo "";
        old="version $oldver"
        new="version $newver"
        echo "Updating spec";
        perl -p -i.bak -e s/"$old"/"$new"/g socnetv.spec
	echo "Please update socnetv.spec changelog manually"
	
}



echo .
echo ---------------------------------
echo !  UPDATE VERSION IN ALL FILES   !
echo ---------------------------------


echo "Update version numbers in all files? (Y/N)"
read ans
if [ $ans = "Y" ]; then
	update_version;
elif [ $ans = "y" ]; then
       update_version;
fi

echo "";
echo "Trying to update build date..."

oldbuild=`grep -h "BUILD=" src/mainwindow.cpp`

echo $oldbuild
newdate=`LC_TIME=en_US.UTF-8 date`;
newbuild="QString BUILD=\"$newdate\";"
echo "to this..."
echo $newbuild
perl -p -i.bak -e s/"$oldbuild"/"$newbuild"/g src/mainwindow.cpp


echo "";
echo "cleaning up temp files...";
if [ -d ../temp ]; then
	rm -rf ../temp/*
else
	mkdir ../temp
fi

# mv src/*.bak ../temp	
find . -type f -name "*.bak" -exec mv {} ../temp \;

