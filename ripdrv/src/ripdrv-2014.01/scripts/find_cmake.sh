#!/bin/bash

# set CMAKE and CTEST vars to name cmake and ctest 2.8 executables
# if these are already set, their value is used and no search is performed

function cmake_version()
{
  local exe=$1
  if [ -x $exe ]; then
    echo $($exe --version | cut -d' ' -f3 | cut -d- -f1 |cut -d. -f1-2)
  fi
}

function find_cmake_exe()
{
  local basename=$1
  local version=$2

  if [ -z $basename ]; then
    basename=cmake
  fi

  if [ -z $version ]; then
    version=2.8
  fi

  #list of all executable to check
  CHECK="$(which $basename) $basename$(echo $version | tr -d .) /usr/bin/$basename /usr/bin/$basename$(echo $version | tr -d .)"

  for CM in $CHECK; do
    V=$(cmake_version $CM)
    if [ "$V" == "$version" ]; then
      echo $CM
      return 0
    fi
  done
}

if [ -z $CMAKE ]; then
  CMAKE=$(find_cmake_exe cmake 2.8)
  if [ -z $CMAKE ]; then
    echo "cmake 2.8 not found"
    exit 1
  fi
fi

if [ -z $CTEST ]; then
  CTEST=$(find_cmake_exe ctest 2.8)
  if [ -z $CTEST ]; then
    echo "ctest 2.8 not found"
    exit 1
  fi
fi

export CMAKE
export CTEST

