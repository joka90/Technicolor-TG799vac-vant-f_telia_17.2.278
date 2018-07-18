#!/bin/bash

unset CDPATH

# check if lcov is installed
hash lcov 2>/dev/null || { echo "lcov not installed"; exit 1; }

TOP=$PWD
ROOT=$(cd $(dirname $0); cd ..; pwd)

# get the CMAKE and CTEST
. $ROOT/scripts/find_cmake.sh

# this must be relative to the current dir.
# at least until after prep.sh has been called
TESTDIR=.runtest

$ROOT/scripts/prep.sh $TESTDIR

cd $TESTDIR

# build the tests
make
E=$?
if [ $E -ne 0 ]; then
  exit $E
fi

# remove any leftover coverage files
find -name '*gcno' -o -name '*gcda' -delete
rm -rf $TOP/coverage

# run the test
$CTEST -V
E=$?
if [ $E -ne 0 ]; then
  exit $E
fi

# gather coverage info
COVDIRS=""
for d in $(find -name '*gcno' -printf '%h\n' | sort | uniq); do
  COVDIRS="$COVDIRS -d $d/"
done

lcov -c $COVDIRS -o coverage.info
genhtml coverage.info --output-directory $TOP/coverage --no-branch-coverage --legend --title "RIP driver"

# tidy up xml result files
for result in `find -name "testresults-*.xml"`; do
  sed 's/xmlns=/ignore=/g' ${result} > ${result}.cleaned
  xsltproc --output $(basename ${result}) $TOP/$TESTDIR/dl/testing/Technicolor/common/check2junit.xslt ${result}.cleaned
  rm ${result}.cleaned
done
