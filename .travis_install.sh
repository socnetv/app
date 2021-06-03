#!/bin/bash

echo "******************"
echo "Installing Qt5..."
echo "******************"


# Check current directory
project_dir=$(pwd)
echo "Project dir is: ${project_dir}"


if [ "${TRAVIS_OS_NAME}" == "linux" ]; then
    sudo apt-get install mesa-common-dev
    sudo apt-get install build-essential libgl1-mesa-dev
    # You need to change this if you need to update to a more recent Qt version
    sudo apt-get -y install qt512base qt512charts-no-lgpl  qt512svg
    source /opt/qt512/bin/qt512-env.sh


elif [ "${TRAVIS_OS_NAME}" == "osx" ]; then
    # We install Qt5 via brew
    # Note we use qt@5 to install Qt 5.15.2 because `brew install qt` would install Qt 6 by default...
    brew install qt@5 p7zip
    brew link --force qt@5
    # Add Qt binaries to path
    PATH=/usr/local/opt/qt/bin/:${PATH}

else
	exit 1
fi

echo "Done!"

exit 0
