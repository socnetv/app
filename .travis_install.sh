#!/bin/bash

echo "******************"
echo "Installing Qt5..."
echo "******************"


# Check current directory
project_dir=$(pwd)
echo "Project dir is: ${project_dir}"


if [ "${TRAVIS_OS_NAME}" == "linux" ]; then
    sudo apt-get install mesa-common-dev
#    sudo apt install libglu1-mesa-dev freeglut3-dev
    sudo apt-get install build-essential libgl1-mesa-dev
    sudo apt-get -y install qt515base qt515charts-no-lgpl  qt515svg
    # sudo apt-get install qt5-default qttools5-dev qttools5-dev-tools qtbase5-dev qtbase5-dev-tools qttranslations5-l10n libqt5svg5-dev
    source /opt/qt515/bin/qt515-env.sh


elif [ "${TRAVIS_OS_NAME}" == "osx" ]; then
	# We install Qt5 via brew
    brew install qt p7zip
    brew link --force qt
    # Add Qt binaries to path
    PATH=/usr/local/opt/qt/bin/:${PATH}

else
	exit 1
fi

echo "Done!"

exit 0
