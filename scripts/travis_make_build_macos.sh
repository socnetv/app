#!/bin/bash


set -e # Exit on any error
set -o pipefail # Ensure errors in pipelines are caught

echo "************************************************"
echo "* STAGE 'script':  Building SocNetV for macOS  *"
echo "************************************************"

# Constants
APP_NAME="SocNetV"
project_dir=$(pwd)
LAST_COMMIT_SHORT=$(git rev-parse --short HEAD)
# linuxdeployqt always uses the output of 'git rev-parse --short HEAD' as the version.
# We change this as follows:
# If this is a tag, then version will be the tag, i.e. 2.5 or 2.5-beta
# If this is not a tag, the we want version to be like "2.5-beta-a0be9cd"
VERSION=${TRAVIS_TAG:-"${SOCNETV_VERSION}-${LAST_COMMIT_SHORT}"}

# Print basic vars
echo "PATH: $PATH"
echo "TRAVIS_OS_NAME = $TRAVIS_OS_NAME"
echo "TRAVIS_TAG = $TRAVIS_TAG"
echo "TRAVIS_COMMIT = $TRAVIS_COMMIT"
echo "SOCNETV_VERSION = $SOCNETV_VERSION"
echo "TAG_NAME = ${TAG_NAME}"


echo "Project dir: ${project_dir}"
echo "Building version: ${VERSION}"

# Ensure Qt is installed and in PATH
export PATH="$(brew --prefix qt)/bin:$PATH"

if ! command -v qmake &> /dev/null; then
    echo "Error: qmake not found. Ensure Qt is installed via Homebrew."
    exit 1
fi

echo "qmake version:"
qmake -version

# Print system info
echo "macOS version: $(sw_vers -productVersion)"
echo "Xcode SDK Path: $(xcrun -sdk macosx --show-sdk-path)"


# Print build env
echo "Xcode build version = "
xcrun -sdk macosx --show-sdk-path



# Build the app
echo "*****************************"
echo "Building ${APP_NAME} ${VERSION}..."
echo "*****************************"

echo "Entering project dir:"
cd ${project_dir}

echo "Running qmake to configure it as release..."
qmake QMAKE_APPLE_DEVICE_ARCHS="x86_64 arm64" -config release || {
    echo "Error: qmake failed."
    exit 1
}

echo "Running make to compile the source code..."
make -j$(sysctl -n hw.ncpu) || {
    echo "Error: make failed."
    exit 1
}


echo "Build complete. Verifying contents..."
find . -type f -name "${APP_NAME}*"


# Package the app
echo "*****************************"
echo "Packaging ${APP_NAME} ${VERSION}..."
echo "*****************************"

echo "Entering project dir ${project_dir} ..."
cd ${project_dir}/


# Remove build directories that we don't want to deploy
echo "Removing items we do not deploy from project dir ${project_dir}..."
rm -rf moc obj qrc

# echo "Contents of ${APP_NAME}.app:"
# find ${APP_NAME}.app -type f

echo "Calling macdeployqt to create dmg archive from ${APP_NAME}.app:"
macdeployqt "${APP_NAME}.app" -dmg || {
    echo "Error: macdeployqt failed."
    exit 1
}

# Ensure universal binaries (x86_64 and arm64) are consistently included, using lipo:
lipo -info ${APP_NAME}.app/Contents/MacOS/${APP_NAME}


# echo "Finished macdeployqt -- ${APP_NAME}.app now has these files inside:"
# find ${APP_NAME}.app -type f

# Rename and package
DMG_FILE="${APP_NAME}-${VERSION}.dmg"
echo "Rename dmg archive to ${DMG_FILE} ..."
if [[ -f ${APP_NAME}.dmg ]]; then
    mv "${APP_NAME}.dmg" "${DMG_FILE}"
else
    echo "Error: DMG creation failed. No DMG file found."
    exit 1
fi

echo "Verifying DMG contents:"
hdiutil attach ./${APP_NAME}-${VERSION}.dmg -nobrowse || exit 1

# Create the zipped archive
7z a "${APP_NAME}-${VERSION}-macos.zip" "${DMG_FILE}" README.md COPYING || {
    echo "Error: Failed to create zip archive."
    exit 1
}


# Remove the first DMG file
[[ -f ${APP_NAME}-${VERSION}.dmg ]] && rm ${APP_NAME}-${VERSION}.dmg


if ! command -v create-dmg &> /dev/null; then
    echo "create-dmg not found. Skipping alternate DMG creation step."
else
    echo "Using create-dmg for additional DMG creation..."
    echo "Creating release subdirectory..."
    mkdir -p release


    echo "Moving app and extra files inside the release subdirectory..."
    mv ${APP_NAME}.app release/
    cp README.md release/
    cp LICENSE release/

    # --background $project_dir/packaging/macosx/DMG-Background.png \

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

        echo "Verifying DMG contents:"
        hdiutil attach ./${APP_NAME}-${VERSION}.dmg -nobrowse || exit 1

fi




echo "Release files:"
find . -type f -name "${APP_NAME}*"

echo "************************************************"
echo "* Build and packaging completed successfully.  *"
echo "************************************************"

exit 0