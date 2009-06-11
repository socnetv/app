#!/bin/bash
workdir=/media/oxy/Code/Qt/socnetv/trunk
cd $workdir


update_version() {
	old=0.6;
	new=0.70;
	dateold=2008;
	datenew=2009;
	echo "Current version seems to be ....";
	grep "VERSION=" src/mainwindow.cpp

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
	

	echo "";
	old="v. $oldver"
	new="v. $newver"
	echo "Updating manual footer";
	perl -p -i.bak -e s/"$old"/"$new"/g manual/footer.html

	echo "";
	echo "Updating configure";
	perl -p -i.bak -e s/"$oldver"/"$newver"/g configure.ac

	echo "";
	echo "Running autoconf..."
	autoconf &> /dev/null
	
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
newdate=`date`;
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

mv src/*.bak ../temp	


