#!/bin/bash


echo  --------------------
echo  . copying webfiles .
echo  --------------------
cd /media/oxy/Code/Qt/socnetv/site
rsync -rvPC -delete -e ssh  *  oxy86,socnetv@web.sourceforge.net:htdocs/



echo "Update Manual also? (Y/N)"
read ans
if [ $ans = "N" ]; then
        exit;

elif [ $ans = "n" ]; then
        exit;
fi

echo  --------------------
echo  . copying manual   .
echo  -------------------- 
cd /media/oxy/Code/Qt/socnetv/trunk/manual
rm /media/oxy/Code/Qt/socnetv/site/docs/*
cp /media/oxy/Code/Qt/socnetv/trunk/manual/* /media/oxy/Code/Qt/socnetv/site/docs/

rsync -rvP -delete -e ssh * oxy86,socnetv@web.sourceforge.net:htdocs/docs/
