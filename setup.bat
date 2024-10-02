@echo off
setlocal

REM Check if vcpkg is already downloaded
IF NOT EXIST ".vcpkg" (
    echo Downloading vcpkg...
    git clone https://github.com/microsoft/vcpkg.git .vcpkg
    cd .vcpkg
    echo Bootstrapping vcpkg...
    .\bootstrap-vcpkg.bat
    echo Removing .git and .github folders...
    rmdir /s /q .git
    rmdir /s /q .github
    cd ..
) ELSE (
    echo vcpkg is already downloaded.
)

REM Install vcpkg required libraries
cd .vcpkg

REM Install libraries
echo Installing websocketpp:x64-windows...
.\vcpkg.exe install websocketpp:x64-windows

echo Installing mongo-cxx-driver:x64-windows...
.\vcpkg.exe install mongo-cxx-driver:x64-windows

echo Installing cpp-jwt:x64-windows...
.\vcpkg.exe install cpp-jwt:x64-windows

echo Installing rapidjson:x64-windows...
.\vcpkg.exe install rapidjson:x64-windows

echo Installing Boost:x64-windows...
.\vcpkg.exe install boost:x64-windows

echo All libraries installed.

cd ..
echo Setup complete.
pause
