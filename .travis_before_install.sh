#!/bin/bash

echo "Adding Qt5 repos and updating platform...";

if [ "${TRAVIS_OS_NAME}" == "linux" ]; then
    sudo apt-get -qq update
    sudo add-apt-repository ppa:beineri/opt-qt58-trusty -y
    sudo apt-get update -qq

elif [ "${TRAVIS_OS_NAME}" == "osx" ]; then
	# We install Qt5 via brew,
	brew update
else
	exit 1
fi

echo "Done!"

exit 0