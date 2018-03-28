#!/bin/bash
if [[ $0 != "./build.sh" ]]; then
    echo "Must run from same directory as source!"
    exit 1
fi

if [ $# -lt 1 ]; then
    echo "Enter version name/number"
    exit 1
fi

echo "Version name/number: $1"

if [[ -z $CORES ]]; then
    CORES=1
fi
echo "Using $CORES cores"

# Clear build space
rm -rf build
rm -rf release

# Create release directory
mkdir release

# Set Linux C++ compiler (prefer clang++ over g++)
CXX=
if [ -x "$(command -v clang++)" ]; then
    CXX=clang++
    echo "$CXX found for Linux"
elif [ -x "$(command -v g++)" ]; then
    CXX=g++
    echo "$CXX found for Linux"
fi

# Check if no compiler found
if [ -z $CXX ]; then
    echo "No C++ compiler for Linux!"
    exit 1
fi
echo "Using $CXX for Linux 64-bits & 32-bits"

# Set Linux 32-bit and 64-bit C++ compilers
ARCHS=("-m32" "-m64" "" "")
COMPILERS=($CXX $CXX)
EXTS=("linux32" "linux64")

# Set Windows 32-bit C++ compiler
if [[ -z $WIN_CXX_32 ]]; then
    WIN_CXX_32=i686-w64-mingw32-g++
    echo "Defaulting to $WIN_CXX_32"
fi
if [ -x "$(command -v $WIN_CXX_32)" ]; then
    COMPILERS+=($WIN_CXX_32)
    EXTS+=("win32.exe")
    echo "$WIN_CXX_32 found for Win32"
    echo "Using compiler $WIN_CXX_32 for Win32"
fi

# Set Windows 64-bit C++ compiler
if [[ -z $WIN_CXX_64 ]]; then
    WIN_CXX_64=x86_64-w64-mingw32-g++
    echo "Defaulting to $WIN_CXX_64"
fi
if [ -x "$(command -v $WIN_CXX_64)" ]; then
    COMPILERS+=($WIN_CXX_64)
    EXTS+=("win64.exe")
    echo "$WIN_CXX_64 found for Win64"
    echo "Using compiler $WIN_CXX_64 for Win64"
fi

echo ${COMPILERS[@]}

i=0
for CXX in ${COMPILERS[@]}; do
    ARCH=${ARCHS[$i]}
    EXT=${EXTS[$i]}

    make \
        CXX=$CXX \
        EXTRACXXFLAGS=$ARCH \
        EXTRALDFLAGS="-static" \
        ENGINE_NAME="Teki $1" \
        -j$CORES
    mv teki release/Teki$1_$EXT
    strip -s release/Teki$1_$EXT
    make clean

    i=$((i+1))
done
