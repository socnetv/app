#!/bin/bash

set -e  # Exit on any error
set -o pipefail  # Ensure errors in pipelines are caught

echo "************************************************"
echo "* STAGE 'script':  Building SocNetV for Linux  *"
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
echo ""

#
# NOTE:
#
# linuxdeployqt always uses the output of 'git rev-parse --short HEAD' as the version.
# We change this by exporting $VERSION environment variable
#

echo "Checking TRAVIS_TAG to fix the VERSION..."
echo "If this is a tag, then version will be the tag, i.e. 3.1 or 3.1-dev"
echo "If this is not a tag, the version will include the LAST_COMMIT_SHORT, i.e. 3.1-beta-a0be9cd"
if [ ! -z "$TRAVIS_TAG" ]; then
    export VERSION=${TRAVIS_TAG}  # Use tag if available
else
    export VERSION=${SOCNETV_VERSION}-${LAST_COMMIT_SHORT}  # Use commit short hash if no tag
fi


echo "exported VERSION = ${VERSION}";


if [ "${TRAVIS_OS_NAME}" == "linux" ]; then

    echo "Building SocNetV AppImage for Linux..."
    
    # System Information
    lsb_release -a
    echo "OpenSSL version: $(openssl version)"

    echo "Checking qmake version..."
    which qmake6
    qmake6 -v

    # Running qmake and build with make
    echo "Running qmake6 to configure project..."
    qmake6  # default configuration for all targets
    echo "Building with 'make -j$(nproc)' (using all available cores)..."
    make -j$(nproc)

  # Output build results
    echo "Build finished! Files in current directory:"
    find .

    # Install to appdir
    echo "Attempting make install in appdir..."
    make INSTALL_ROOT=appdir install
    echo "SocNetV files installed in appdir:"
    find appdir/

    # Verify dependencies
    echo "Checking SocNetV executable libraries:"
    ldd appdir/usr/bin/socnetv

    # Copy application assets
    echo "Copying .desktop and icon files..."
    cp appdir/usr/share/applications/socnetv.desktop ./appdir
    cp appdir/usr/share/pixmaps/socnetv.png .

    #echo "copying custom openssl libs to ./appdir/usr/bin..."
    #cp /opt/openssl-1.1.1/lib/libssl.so.1.1 ./appdir/usr/bin/
    #cp /opt/openssl-1.1.1/lib/libcrypto.so.1.1 ./appdir/usr/bin/


    # Check contents of appdir
    echo "SocNetV files installed in appdir -- final:"
    find appdir/

    # Check Qt plugins
    echo "Checking Qt plugins directory..."
    find /opt/ | grep "/plugins" | grep qt
    echo "Checking /opt directory for Qt:"
    find /opt | grep qt

    
    # Download and run linuxdeployqt tool
    echo "Downloading linuxdeployqt tool: "
    wget --no-verbose  "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
#    echo "Downloading appimagetool tool (in GO): "
#    wget -c https://github.com/$(wget -q https://github.com/probonopd/go-appimage/releases/expanded_assets/continuous -O - | grep "appimagetool-.*-x86_64.AppImage" | head -n 1 | cut -d '"' -f 2)


    # Make linuxdeployqt executable
    echo "Making linuxdeployqt executable..."
    chmod a+x linuxdeployqt*.AppImage

    unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH
    
    # Run linuxdeployqt to bundle dependencies
    echo "Running linuxdeployqt to create AppImage..."
    ./linuxdeployqt*.AppImage appdir/usr/share/applications/*.desktop -appimage -extra-plugins=iconengines,imageformats -qmake=/usr/bin/qmake6

#    echo "Run the appimagetool: "
#    ./appimagetool-*.AppImage -s deploy appdir/usr/share/applications/*.desktop -appimage -extra-plugins=iconengines,imageformats # Bundle EVERYTHING
    # or
    # ./appimagetool-*.AppImage deploy appdir/usr/share/applications/*.desktop # Bundle everything expect what comes with the base system
    # and
    # VERSION=1.0 ./appimagetool-*.AppImage ./Some.AppDir # turn AppDir into AppImage


    # Clean up
    echo "Cleaning up linuxdeployqt tool..."
    rm linuxdeployqt-continuous-x86_64.AppImage

    echo ""
    echo "Check whether the SocNetV AppImage has been created..."
    find . -type f -name "*AppImage"


else
    echo "Error: This script should be running on Linux, but detected TRAVIS_OS_NAME=${TRAVIS_OS_NAME}."
    exit 1
fi


echo "Done!"

# always return zero
exit 0
