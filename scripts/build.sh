#!/bin/bash -e

# Arguments:
#   -b Debug/Release debug or release build, debug if not passed
#   -j Threads to build with, 1 if not passed.

IS_MSYS2_CLANG=off
COMPILER=clang++
CCOMPILER=clang
LINKER=lld
THREADS=1
BUILD_TYPE=Debug

while getopts b:j: opts; do
   case ${opts} in
      b) BUILD_TYPE=${OPTARG} ;;
      j) THREADS=${OPTARG} ;;
   esac
done

# Go to build dir
CURDIR=$(basename $PWD)
BUILD_DIR="build/${CCOMPILER}_${BUILD_TYPE,,}"
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
WORKSPACE=${SCRIPTPATH%/*}
if [[ $CURDIR == "scripts" ]]; then
  cd ..
fi
mkdir -p build
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

export CXX=$COMPILER
export CC=$CCOMPILER

CMAKE_GENERATOR="Unix Makefiles"
if [[ ! -z "${MSYSTEM}" ]]; then
  CMAKE_GENERATOR="MSYS Makefiles"
  if [[ $CCOMPILER == clang ]]; then
    IS_MSYS2_CLANG=on
  fi
fi

CMAKE_CXX_FLAGS=""
if [[ ! -z "${MSYSTEM}" ]]; then
  if [[ $CCOMPILER == clang ]]; then
    CMAKE_CXX_FLAGS="-fuse-ld=lld"
  fi
fi

cmake \
  -G"${CMAKE_GENERATOR}" \
  -DMSYS2_CLANG=$IS_MSYS2_CLANG \
  -DNUI_BUILD_EXAMPLES=on \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
  -DCMAKE_CXX_EXTENSIONS=on \
  -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  -DCMAKE_CXX_FLAGS=$CMAKE_CXX_FLAGS \
  -DCMAKE_CXX_COMPILER=$COMPILER \
  -DCMAKE_C_COMPILER=$CCOMPILER \
  -DCMAKE_LINKER=$LINKER \
  -DCMAKE_CXX_STANDARD=23 \
  ../..

# if [ ! -d ./_deps/emscripten-src/upstream ]; then
#   make emscripten_setup
# fi

cd ../..
node ./scripts/copy_compile_commands.js
cd ${BUILD_DIR}
make -j$THREADS
