#!/bin/bash

echo "*****************************"
echo "Building SocNetV for macOS..."
echo "*****************************"

APP_NAME="SocNetV"


# Check current directory
project_dir=$(pwd)
echo "Project dir is: ${project_dir}"

# Check version
VERSION=`git rev-parse --short HEAD`
echo "Version is: ${VERSION}"

echo "TRAVIS_TAG is: $TRAVIS_TAG"
echo "TRAVIS_COMMIT is: $TRAVIS_COMMIT"

# Output macOS version
echo "macOS version is:"
sw_vers

# Install npm appdmg if you want to create custom dmg files with it
# npm install -g appdmg


# Build your app
echo "*****************************"
echo "Start building NOW..."
echo "*****************************"

cd ${project_dir}
qmake -config release
make -j4

# Build and run your tests here
find .

# Package your app
echo "*****************************"
echo "Packaging ${APP_NAME}..."
echo "*****************************"
cd ${project_dir}/

# Remove build directories that you don't want to deploy
rm -rf moc
rm -rf obj
rm -rf qrc

echo "Creating dmg archive..."
echo "TAG_NAME is ${TAG_NAME}"

macdeployqt ${APP_NAME}.app -dmg
mv ${APP_NAME}.dmg "${APP_NAME}_${VERSION}.dmg"

# You can use the appdmg command line app to create your dmg file if
# you want to use a custom background and icon arrangement. I'm still
# working on this for my apps, myself. If you want to do this, you'll
# remove the -dmg option above.
# appdmg json-path ${APP_NAME}_${TRAVIS_TAG}.dmg

# Copy other project files
cp "${project_dir}/README.md" "README.md"
cp "${project_dir}/COPYING" "LICENSE"

echo "Packaging zip archive..."
7z a ${APP_NAME}_${VERSION}_macos.zip "${APP_NAME}_${VERSION}.dmg" "README.md" "LICENSE"

echo "Check what we have created..."
find . -type f -name "${APP_NAME}*"

echo "Done!"

exit 0