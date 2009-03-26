#!/bin/bash


echo  --------------------
echo  . copying webfiles .
echo  --------------------
cd ~/Documents/socnetv/site
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
cd ~/Documents/socnetv/trunk/manual
rm ~/Documents/socnetv/site/docs/*
cp ~/Documents/socnetv/trunk/manual/* ~/Documents/socnetv/site/docs/

rsync -rvP -delete -e ssh * oxy86,socnetv@web.sourceforge.net:htdocs/docs/
