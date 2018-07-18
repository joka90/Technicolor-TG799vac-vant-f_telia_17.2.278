#!/bin/bash

unset CDPATH

TOP=$PWD

SCRIPTDIR=$(cd $(dirname $0); pwd)

. $SCRIPTDIR/find_cmake.sh

GIT=${GIT:-git}

BUILD=$(echo $PWD/${1:-build})
INSTALL=$BUILD/install
DL=$BUILD/dl
mkdir -p $BUILD
mkdir -p $INSTALL
mkdir -p $DL
cd $BUILD

function build_and_install() {
  local package=$1
  shift
  mkdir -p $BUILD/deps/$package
  pushd $BUILD/deps/$package
  ${CMAKE} -DCMAKE_FIND_ROOT_PATH=$INSTALL -DCMAKE_INSTALL_PREFIX=$INSTALL $@ $DL/$package
  make install
  popd
}

function update_or_clone() {
  mkdir -p $DL
  pushd $DL
  if [ ! -d "$1" ]; then
    ${GIT} clone $2 $1
  else
    cd $1
    ${GIT} pull
  fi
  popd
}

# get and build dependencies
update_or_clone testing git@gitolite.edegem.eu.thmulti.com:/testing.git
build_and_install testing/Technicolor/unittestCheck


$CMAKE -DCMAKE_FIND_ROOT_PATH=$INSTALL -DCMAKE_INSTALL_PREFIX=$INSTALL -DCMAKE_BUILD_TYPE=Debug $TOP

