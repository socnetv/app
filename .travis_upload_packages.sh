#!/bin/bash

echo "************************************************"
echo "* STAGE 'after_success':  Uploading packages   *"
echo "************************************************"

# Check current directory
project_dir=$(pwd)
echo "Project dir is: ${project_dir}"

echo "TRAVIS_OS_NAME: $TRAVIS_OS_NAME";

if [ ! -z "${TRAVIS_TAG}" ]; then
    echo "TRAVIS_TAG: $TRAVIS_TAG";
    echo "The upload tool will upload files to tag TRAVIS_TAG";
elif [ -z "$TRAVIS_TAG" ] ; then
    echo "TRAVIS_TAG is empty.";
    echo "The upload tool will upload files to 'continuous' CI/CD pipeline";
fi

echo " Downloading upload.sh tool. Please wait...";
wget --no-verbose https://github.com/probonopd/uploadtool/raw/master/upload.sh

if [ "${TRAVIS_OS_NAME}" == "linux" ]; then
    echo "Listing any *SocNetV*.AppImage* package files for Linux...";
    ls -lsh  *SocNetV*.AppImage*
    echo "Using upload.sh tool to upload binaries to GitHub..."
    bash upload.sh *SocNetV*.AppImage*

elif [ "${TRAVIS_OS_NAME}" == "osx" ]; then
    echo "Listing any *SocNetV*.* package files for macOS...";
    ls -lsh  *SocNetV*.*
    bash upload.sh *SocNetV*.zip*
    bash upload.sh SocNetV*.dmg*;

else
	exit 1
fi

echo ""
echo "travis_upload_packages.sh: DONE uploading our packages. Returning now to main script."
echo ""

exit 0
