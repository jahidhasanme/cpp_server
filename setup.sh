#!/bin/bash

# Enable error handling and exit on failure
set -e

# Check if vcpkg is already downloaded
if [ ! -d ".vcpkg" ]; then
    echo "Downloading vcpkg..."
    git clone https://github.com/microsoft/vcpkg.git .vcpkg
    cd .vcpkg
    echo "Bootstrapping vcpkg..."
    ./bootstrap-vcpkg.sh
    echo "Removing .git and .github folders..."
    rm -rf .git
    rm -rf .github
    cd ..
else
    echo "vcpkg is already downloaded."
fi

# Install vcpkg required libraries
cd .vcpkg

# Install libraries
echo "Installing websocketpp:x64-linux..."
./vcpkg install websocketpp:x64-linux

echo "Installing mongo-cxx-driver:x64-linux..."
./vcpkg install mongo-cxx-driver:x64-linux

echo "Installing cpp-jwt:x64-linux..."
./vcpkg install cpp-jwt:x64-linux

echo "Installing rapidjson:x64-linux..."
./vcpkg install rapidjson:x64-linux

echo "Installing Boost:x64-linux..."
./vcpkg install boost:x64-linux

echo "All libraries installed."

cd ..
echo "Setup complete."