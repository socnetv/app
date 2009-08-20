#!/bin/bash
workdir=/home/dimitris/Desktop/socnetv/trunk
cd $workdir

echo ---------------------------------
echo ! COMMIT ALL CHANGES TO SVN     !
echo ---------------------------------
echo .

echo "Update version and build date before commiting? (Y/N)"
read ans
if [ $ans = "Y" ]; then
        ../tools/update-version.sh

elif [ $ans = "y" ]; then
	../tools/update-version.sh
fi

cd $workdir

svn commit
