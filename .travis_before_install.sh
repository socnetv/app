#!/bin/bash

echo "******************************************"
echo "Adding Qt5 repos and updating platform..."
echo "******************************************"

# Check current directory
project_dir=$(pwd)
echo "Project dir is: ${project_dir}"


if [ "${TRAVIS_OS_NAME}" == "linux" ]; then
    #
    # Install base Qt5 repos for linux
    # NOTE: Changed the add-apt-repository line to update to a newer Qt version
    #
    echo "installing base Qt5 repos for Ubuntu from ppa:beineri"
    sudo apt-get -qq update
    sudo add-apt-repository ppa:beineri/opt-qt-5.12.10-xenial -y
    sudo apt-get update -qq

elif [ "${TRAVIS_OS_NAME}" == "osx" ]; then
    #
    # Update brew, we will use it ltr to install Qt5
    #
    echo "Updating brew..."
    brew update
    echo "Finished updating brew."
else
    exit 1
fi

echo ""
echo "travis_before_install.sh: DONE. Returning now to main script."
echo ""

exit 0
