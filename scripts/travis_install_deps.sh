#!/bin/bash

echo "********************************************************"
echo "* STAGE 'install':  Installing dependencies (Qt6, etc) *"
echo "********************************************************"

# Check current directory
project_dir=$(pwd)
echo "Project dir is: ${project_dir}"


if [ "${TRAVIS_OS_NAME}" == "linux" ]; then
    #
    # Install required development packages
    #
    echo "Installing required linux development packages..."
    sudo apt-get install mesa-common-dev
    sudo apt-get install build-essential libgl1-mesa-dev
    #
    # Install Qt6 packages and configure host environment
    #
    echo "Installing Qt6 packages..."
    # You need to change this if you need to update to a more recent Qt version
    sudo apt-get -y install qt6-base-dev qt6-base-dev-tools qt6-tools-dev libqt6charts6-dev libqt6svg6-dev libqt6core5compat6-dev libqt6opengl6-dev
    sudo apt install libfuse2 # otherwise Appimage will not start in Ubuntu 22.04 -- https://github.com/OpenShot/openshot-qt/issues/4789
    echo ""
    echo "Is there a script to set it up in the host system??..."
    ls /opt/
    #source /opt/qt512/bin/qt512-env.sh
    echo "Check qtchooser -l"
    qtchooser -l
    echo "In Ubuntu 22.04 there is a qtchooser bug (see 'QtChooser doesnt support qt6') "
    echo "so qtchooser -l does not list any qt6 option."
    echo "So, we must manually select Qt6 for current user only."
    echo "First, we generate qt6.conf based on the path to qmake6..."
    qtchooser -install qt6 $(which qmake6)
    echo "Selecting Qt6 as default with export QT_SELECT=qt6 (also placing it in ~/.bashrc for persistence):"
    export QT_SELECT=qt6
    echo 'export QT_SELECT=qt6' >> ~/.bashrc
    echo "cat ~/.bashrc to check..."
    cat ~/.bashrc
    echo "Finished installing and configuring Qt6 packages..."

elif [ "${TRAVIS_OS_NAME}" == "osx" ]; then
    #
    # Install Qt for macOS via brew and configure host environment
    #
    # echo "Installing Qt6 for macOS via brew..."
    # brew install qt@6
    # echo "installing p7zip"
    # brew install p7zip
    # echo "installing create-dmg"
    # brew install create-dmg
    ## Install npm appdmg if you want to create custom dmg files with it
    # # npm install -g appdmg
    # echo "Running brew link to symlink various Qt binaries into /usr/local/bin etc so..."
    # brew link --force qt@6
    # Add Qt binaries to path
    echo "Adding qt binaries installation path to system PATH..."
    # PATH=/usr/local/opt/qt/bin/:${PATH}       
    # Ensure Homebrew Qt is accessible in PATH
    echo 'export PATH="$(brew --prefix qt)/bin:$PATH"' >> ~/.bash_profile; 
    source ~/.bash_profile;

else
	exit 1
fi

echo ""
echo "travis_install_deps.sh: DONE installing dependencies. Returning now to main script."
echo ""

exit 0
