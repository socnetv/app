#!/bin/bash
old=0.48;
new=0.49;
date-old=2008;
date-new=2009;
perl -p -i.bak -e 's/'$old'/'$new'/g' src/*.cpp 
perl -p -i.bak -e 's/'$old'/'$new'/g' src/*.h
perl -p -i.bak -e 's/'$date-old'/'$date-new'/g' src/*.cpp
perl -p -i.bak -e 's/'$date-old'/'$date-new'/g' src/*.h
mkdir ../temp
mv src/*.bak ../temp

