#!/bin/bash

echo "*****************************"
echo "*     Building SocNetV      *"
echo "*****************************"

# Check current directory
project_dir=$(pwd)
echo "Project dir is: ${project_dir}"
echo ""
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
if [ ! -z "$TRAVIS_TAG" ] ; then
    # If this is a tag, then version will be the tag, i.e. 2.6 or 2.6-beta
    export VERSION=${TRAVIS_TAG}
else 
    # If this is not a tag, the we want version to be like "2.6-beta-a0be9cd"
    export VERSION=${SOCNETV_VERSION}-${LAST_COMMIT_SHORT}
fi

echo "exported VERSION = ${VERSION}";

echo "This VERSION will be used by linuxdeployqt and macdeployqt"

echo ""

if [ "${TRAVIS_OS_NAME}" == "linux" ]; then
    # source /opt/qt512/bin/qt512-env.sh

    echo ""
    echo "I will build a SocNetV AppImage for linux distributions..."
    echo ""

    lsb_release -a

    echo ""
    echo "openssl version: "
    echo `openssl version`
    echo ""

    echo "Check output of 'which qmake6':"
    which qmake6

    echo ""
    echo "Check qmake6 -v:"
    qmake6 -v

    echo ""
    echo "Running qmake6 now: "
    qmake6 # default: all go to /usr

    echo "Start building with 'make -j4'. Please wait..."
    make -j4

    echo ""
    echo "Building finished! "

    echo ""
    echo "Files in current directory: "
    find .

    echo ""
    echo "Attempting make install in appdir: "
    make INSTALL_ROOT=appdir install;

    echo ""
    echo "SocNetV files installed in appdir: "
    find appdir/

    echo ""
    echo "Check SocNetV executable libraries:"
    ldd appdir/usr/bin/socnetv

    echo ""
    echo "Copying .desktop file to ./appdir: "
    cp appdir/usr/share/applications/socnetv.desktop ./appdir

    echo ""
    echo "Copying socnetv.png in current dir: "
    cp appdir/usr/share/pixmaps/socnetv.png .  

    #echo ""
    #echo "copying custom openssl libs to ./appdir/usr/bin..."
    #cp /opt/openssl-1.1.1/lib/libssl.so.1.1 ./appdir/usr/bin/
    #cp /opt/openssl-1.1.1/lib/libcrypto.so.1.1 ./appdir/usr/bin/

    echo ""
    echo "SocNetV files installed in appdir -- final: "
    find appdir/



    echo ""
    echo "Checking contents of /opt/qtXX/plugins: "
    find /opt/qt512/plugins
    echo "Checking contents of /opt/: "
    find /opt | grep qt

    echo ""
    echo "Downloading linuxdeployqt tool: "
    wget --no-verbose  "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
#    echo "Downloading appimagetool tool (in GO): "
#    wget -c https://github.com/$(wget -q https://github.com/probonopd/go-appimage/releases/expanded_assets/continuous -O - | grep "appimagetool-.*-x86_64.AppImage" | head -n 1 | cut -d '"' -f 2)


    echo ""
    echo "Make executable the linuxdeployqt tool: "
    chmod a+x linuxdeployqt*.AppImage
#    chmod +x appimagetool-*.AppImage
    unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH

    echo ""
    echo "Check SocNetV executable libraries (AGAIN):"
    ldd appdir/usr/bin/socnetv

    # export VERSION=... # linuxdeployqt uses this for naming the file
    echo ""
    # echo "Run the linuxdeployqt tool: "
    ./linuxdeployqt*.AppImage appdir/usr/share/applications/*.desktop -appimage -extra-plugins=iconengines,imageformats -qmake=/usr/bin/qmake6

#    echo "Run the appimagetool: "
#    ./appimagetool-*.AppImage -s deploy appdir/usr/share/applications/*.desktop -appimage -extra-plugins=iconengines,imageformats # Bundle EVERYTHING
    # or
    # ./appimagetool-*.AppImage deploy appdir/usr/share/applications/*.desktop # Bundle everything expect what comes with the base system
    # and
    # VERSION=1.0 ./appimagetool-*.AppImage ./Some.AppDir # turn AppDir into AppImage


    echo ""
    echo "Removing linuxdeployqt-continuous-x86_64.AppImage..."
    rm linuxdeployqt-continuous-x86_64.AppImage

    echo ""
    echo "Check whether the SocNetV AppImage has been created..."
    find . -type f -name "*AppImage"


elif [ "${TRAVIS_OS_NAME}" == "osx" ]; then
	# nothing here, go away...
    echo "Strange. I am running in macOS although I am a Linux build script :)"
else
	exit 1
fi

echo "Done!"
# always return zero
exit 0
