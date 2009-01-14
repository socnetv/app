#!/bin/bash
echo  --------------------
echo  . copying webfiles .
echo  --------------------
cd ~/Documents/socnetv/site
rsync -Rv -del -e ssh  *  oxy86,socnetv@web.sourceforge.net:htdocs/
echo  --------------------
echo  . copying manual   .
echo  -------------------- 
cd ~/Documents/socnetv/trunk/doc
rsync -Rv -del -e ssh * oxy86,socnetv@web.sourceforge.net:htdocs/docs/
