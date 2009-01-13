#!/bin/bash
cd Documents/socnetv/site
rsync -R  -e ssh  *  oxy86,socnetv@web.sourceforge.net:htdocs/
