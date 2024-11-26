#!/bin/bash

echo "************************************************"
echo "* STAGE 'script':  Building SocNetV for macOS  *"
echo "************************************************"

# Check current directory
project_dir=$(pwd)
echo "Project dir is: ${project_dir}"

echo "TRAVIS_OS_NAME = $TRAVIS_OS_NAME"
echo "TRAVIS_TAG = $TRAVIS_TAG"
echo "TRAVIS_COMMIT = $TRAVIS_COMMIT"
echo "SOCNETV_VERSION = $SOCNETV_VERSION"
echo "TAG_NAME = ${TAG_NAME}"

LAST_COMMIT_SHORT=$(git rev-parse --short HEAD)
echo "LAST_COMMIT_SHORT = $LAST_COMMIT_SHORT"

#
# NOTE:
#
# linuxdeployqt always uses the output of 'git rev-parse --short HEAD' as the version.
# We change this by exporting $VERSION environment variable
#

echo "Checking TRAVIS_TAG to fix the VERSION..."
if [ ! -z "$TRAVIS_TAG" ] ; then
    # If this is a tag, then version will be the tag, i.e. 2.5 or 2.5-beta
    VERSION=${TRAVIS_TAG}
else 
    # If this is not a tag, the we want version to be like "2.5-beta-a0be9cd"
    VERSION=${SOCNETV_VERSION}-${LAST_COMMIT_SHORT}
fi
echo "VERSION = ${VERSION}";

# Print macOS version
echo ""
echo "macOS version = "
sw_vers

# Print build env
echo "Xcode build version = "
xcrun -sdk macosx --show-sdk-path

APP_NAME="SocNetV"

# Build your app
echo "*****************************"
echo "Building ${APP_NAME} ${VERSION}..."
echo "*****************************"

echo "Entering project dir:"
cd ${project_dir}

echo "Running qmake to configure it as release..."
qmake QMAKE_APPLE_DEVICE_ARCHS="x86_64 arm64" -config release 

echo "Running make to compile the source code..."
make -j4

echo "Finished building!"

echo "Current dir contents now:"
find .


# Package your app
echo "*****************************"
echo "Packaging ${APP_NAME} ${VERSION}..."
echo "*****************************"

echo "Entering project dir ${project_dir} ..."
cd ${project_dir}/


# Remove build directories that you don't want to deploy
echo "Removing items we do not deploy from project dir ${project_dir}..."
rm -rf moc
rm -rf obj
rm -rf qrc

echo "Contents of ${APP_NAME}.app:"
find ${APP_NAME}.app -type f

echo "Calling macdeployqt to create dmg archive from ${APP_NAME}.app:"
macdeployqt ${APP_NAME}.app -dmg

echo "Finished macdeployqt -- ${APP_NAME}.app now has these files inside:"
find ${APP_NAME}.app -type f

echo "Check if ${APP_NAME}.dmg has been created:"
find . -type f -name ${APP_NAME}.dmg

echo "Rename dmg archive to ${APP_NAME}-${VERSION}.dmg ..."
mv ${APP_NAME}.dmg "${APP_NAME}-${VERSION}.dmg"

echo "Check if there is a .dmg file created..."
find . -type f -name *.dmg

# Copy other project files
echo "Copying other project files..."
cp "${project_dir}/README.md" "README.md"
cp "${project_dir}/COPYING" "LICENSE"

echo "Creating zip archive..."
7z a ${APP_NAME}-${VERSION}-macos.zip "${APP_NAME}-${VERSION}.dmg" "README.md" "LICENSE"

# Remove the first DMG file
[[ -f ${APP_NAME}-${VERSION}.dmg ]] && rm ${APP_NAME}-${VERSION}.dmg

echo "Creating another dmg file with create-dmg..."
echo "Creating release subdirectory..."
mkdir -p release


echo "Moving app and extra files inside the release subdirectory..."
mv ${APP_NAME}.app release/
cp README.md release/
cp LICENSE release/

#    --background $project_dir/packaging/macosx/DMG-Background.png \
create-dmg \
    --volname ${APP_NAME}-${VERSION} \
    --volicon $project_dir/src/images/socnetv.icns \
    --window-pos 200 120 \
    --window-size 800 400 \
    --icon-size 100  \
    --icon "${APP_NAME}" 200 205 \
    --hide-extension "${APP_NAME}.app" \
    --app-drop-link 600 205 \
    ./${APP_NAME}-${VERSION}.dmg \
    ./release

#echo "Calling productbuild to create product archive .pkg from ${APP_NAME}.app:"
#productbuild --component ${APP_NAME}.app /Applications "${APP_NAME}-${VERSION}.pkg"

# You can use the appdmg command line app to create your dmg file if
# you want to use a custom background and icon arrangement. I'm still
# working on this for my apps, myself. If you want to do this, you'll
# remove the -dmg option above.
# appdmg json-path ${APP_NAME}_${TRAVIS_TAG}.dmg

echo "Check what we have created..."
find . -type f -name "${APP_NAME}*"

echo " "
echo "travis_build_macos: DONE. Returning to main script..."
echo " "

exit 0
