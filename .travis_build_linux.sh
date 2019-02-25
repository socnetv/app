#!/bin/bash

echo "*****************************"
echo "Building SocNetV for linux..."
echo "*****************************"

# Check current directory
project_dir=$(pwd)
echo "Project dir is: ${project_dir}"


if [ "${TRAVIS_OS_NAME}" == "linux" ]; then
    source /opt/qt58/bin/qt58-env.sh
    qmake # default: all go to /usr
    make -j4
    find .
    make INSTALL_ROOT=appdir install; find appdir/
    cp appdir/usr/share/applications/socnetv.desktop ./appdir
    cp appdir/usr/share/pixmaps/socnetv.png .  
    find /opt/qt58/plugins
    wget -c "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage" 
    chmod a+x linuxdeployqt*.AppImage
    unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH
    # - ./linuxdeployqt*.AppImage /usr/share/applications/*.desktop -bundle-non-qt-libs
    ./linuxdeployqt*.AppImage appdir/usr/share/applications/*.desktop -appimage -extra-plugins=iconengines,imageformats

elif [ "${TRAVIS_OS_NAME}" == "osx" ]; then
	# nothing
    echo "Strange..."
else
	exit 1
fi

echo "Done!"
# always return zero
exit 0