#!/bin/bash


echo  --------------------
echo  . copying webfiles .
echo  --------------------
cd /media/oxy/socnetv/site
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
cd /media/oxy/socnetv/trunk/manual
rm /media/oxy/socnetv/site/docs/*
cp /media/oxy/socnetv/trunk/manual/* /media/oxy/socnetv/site/docs/

rsync -rvP -delete -e ssh * oxy86,socnetv@web.sourceforge.net:htdocs/docs/
