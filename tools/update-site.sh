#!/bin/bash
echo  --------------------
echo  . copying webfiles .
echo  --------------------
cd ~/Documents/socnetv/site
rsync -rvP -del -e ssh  *  oxy86,socnetv@web.sourceforge.net:htdocs/



echo "Copy Manual also? (Y/N)"
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
