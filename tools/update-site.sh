#!/bin/bash


echo  --------------------
echo  . copying webfiles .
echo  --------------------
cd ~/Documents/socnetv/site
rm ~/Documents/socnetv/site/docs/*
cp ~/Documents/socnetv/trunk/doc/* ~/Documents/socnetv/site/docs/
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
cd ~/Documents/socnetv/trunk/doc
rsync -rvP -del -e ssh * oxy86,socnetv@web.sourceforge.net:htdocs/docs/
