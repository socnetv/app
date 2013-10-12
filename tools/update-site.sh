#!/bin/bash


echo  --------------------
echo  . copying webfiles .
echo  --------------------

cd ~/socnetv/site
echo "Update ChangeLog also? (Y/N)"
read ans
if [ $ans = "y" -o $ans = "Y" ]; then
        cp  ../trunk/ChangeLog ../site/
fi
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
cd ~/socnetv/trunk/manual
rm ~/socnetv/site/docs/*
cp ~/socnetv/trunk/manual/* ~/socnetv/site/docs/

rsync -rvP -delete -e ssh * oxy86,socnetv@web.sourceforge.net:htdocs/docs/
