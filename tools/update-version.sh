#!/bin/bash
old=0.46;
new=0.47;
perl -p -i.bak -e 's/'$old'/'$new'/g' src/*.cpp 
perl -p -i.bak -e 's/'$old'/'$new'/g' src/*.h
mkdir ../temp
mv src/*.bak ../temp

