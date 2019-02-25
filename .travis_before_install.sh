#!/bin/bash

echo "******************************************"
echo "Adding Qt5 repos and updating platform..."
echo "******************************************"

# Check current directory
project_dir=$(pwd)
echo "Project dir is: ${project_dir}"


if [ "${TRAVIS_OS_NAME}" == "linux" ]; then
	# Install base Qt5 repos for linux
    sudo apt-get -qq update
    sudo add-apt-repository ppa:beineri/opt-qt58-trusty -y
    sudo apt-get update -qq

elif [ "${TRAVIS_OS_NAME}" == "osx" ]; then
	# Update brew, we will use it ltr to install Qt5
	brew update
else
	exit 1
fi

echo "Done!"

exit 0