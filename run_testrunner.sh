#!/bin/sh
#set -x

TESTRUNNER="pytest_c_testrunner"
TESTRUNNER_PATH="${TESTRUNNER}.patch"

git submodule add -b master --name pytest_c_testrunner https://github.com/jmcnamara/pytest_c_testrunner.git pytest_c_testrunner

rm -rf ${TESTRUNNER}
git submodule update --init --recursive
cd ${TESTRUNNER}
rm -rf *
git clean -fdx
git reset --hard HEAD
git apply --reject ../${TESTRUNNER_PATH}
make
pytest $@

