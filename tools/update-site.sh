#!/bin/bash


echo  --------------------
echo  . copying webfiles .
echo  --------------------

cd ~/socnetv/trunk/
echo "Update ChangeLog ? (Y/N)"
read ans
if [ $ans = "y" -o $ans = "Y" ]; then
	scp ChangeLog oxy86,socnetv@web.sourceforge.net:htdocs/
fi





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

rsync -rvP -delete -e ssh * oxy86,socnetv@web.sourceforge.net:htdocs/docs/
