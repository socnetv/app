#!/bin/bash

echo "******************"
echo "Installing Qt5..."
echo "******************"

# Check current directory
project_dir=$(pwd)
echo "Project dir is: ${project_dir}"


if [ "${TRAVIS_OS_NAME}" == "linux" ]; then
    #
    # Install required development packages
    #
    echo "Installing required linux development packages..."
    sudo apt-get install mesa-common-dev
    sudo apt-get install build-essential libgl1-mesa-dev
    #
    # Install Qt5 packages and configure host environment
    #
    echo "Installing Qt5 packages..."
    # You need to change this if you need to update to a more recent Qt version
    sudo apt-get -y install qt512base qt512charts-no-lgpl  qt512svg
    echo "Running Qt5 env script to set it up in the host system..."
    source /opt/qt512/bin/qt512-env.sh
    echo "Finished installing and configuring Qt5 packages..."

elif [ "${TRAVIS_OS_NAME}" == "osx" ]; then
    #
    # Install Qt5 for macOS via brew and configure host environment
    #
    echo "Installing Qt5 for macOS via brew..."
    # Note we use qt@5 to install Qt 5.15.2 because `brew install qt` would install Qt 6 by default...
    brew install qt@5 p7zip
    echo "Running brew link to symlink various Qt5 binaries into /usr/local/bin etc so..."
    brew link --force qt@5
    echo "Adding qt binaries installation path to system PATH..."
    # Add Qt binaries to path
    PATH=/usr/local/opt/qt/bin/:${PATH}

else
	exit 1
fi

echo ""
echo "travis_install.sh: DONE installing Qt packages. Returning now to main script."
echo ""

exit 0
