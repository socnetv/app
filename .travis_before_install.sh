#!/bin/bash

echo "******************************************"
echo "Adding extra repos and updating platform..."
echo "******************************************"

# Check current directory
project_dir=$(pwd)
echo "Project dir is: ${project_dir}"


if [ "${TRAVIS_OS_NAME}" == "linux" ]; then
    #
    # OPTIONAL: Install repos for linux (Qt6, openSSL etc)
    #
    echo "Installing third-party Qt6 repo (to allow using focal as linuxdeployqt wants https://github.com/probonopd/linuxdeployqt/issues/377)..."
    sudo apt-get -qq update
    sudo add-apt-repository -y ppa:okirby/qt6-backports
    sudo apt-get update -qq

    #echo "Downloading openSSL 1.1.1k sources..."
    #wget  --no-verbose "https://github.com/openssl/openssl/archive/refs/tags/OpenSSL_1_1_1k.tar.gz"
    #echo "Unzipping openSSL sources..."
    #tar zxfv OpenSSL_1_1_1k.tar.gz
    #echo "Entering openSSL sources dir..."
    #cd openssl-OpenSSL_1_1_1k/
    #echo "Entering openSSL sources dir..."
    #echo "we are going to build openssl 1.1.1k from source using following setup:"
    #echo "# ./config shared --prefix=/opt/openssl-1.1.1/ && make --jobs=\`nproc --all\` && sudo make install"
    #./config shared --prefix=/opt/openssl-1.1.1/ && make --jobs=`nproc --all` && sudo make install
    #echo "Exiting openSSL sources dir..."
    #cd ..
    #echo "Removing openSSL sources dir..."
    #rm -rf openssl-OpenSSL_1_1_1k/
    #echo "Verifying openSSL installed in /opt/openssl-1.1.1/..."
    #find /opt/openssl-1.1.1/
    #echo "addin openssl libraries to build env"
    #echo "# export LD_LIBRARY_PATH=\"/opt/openssl-1.1.1/lib/:\$LD_LIBRARY_PATH\""
    #export LD_LIBRARY_PATH="/opt/openssl-1.1.1/lib/:$LD_LIBRARY_PATH"
elif [ "${TRAVIS_OS_NAME}" == "osx" ]; then
    #
    # Update brew, we will use it later to install Qt6
    #
    echo "Updating brew..."
    brew update
    echo "Finished updating brew."
else
    exit 1
fi

echo ""
echo "travis_before_install.sh: DONE. Returning now to main script."
echo ""

exit 0
