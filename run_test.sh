#!/bin/sh
#set -x

 ./run_testrunner.sh $@

PROJECT_NAME="sst25vfxxxx_spi"
TESTRUNNER="pytest_c_testrunner"

rm -rf build
mkdir build
cd build
cmake ..
make
cp ../${TESTRUNNER}/conftest.py conftest.py.new
sed -i "s/if path.ext == \".c\" and path.basename.startswith(\"test_\"):/if path.basename.startswith(\"${PROJECT_NAME}\"):/g" conftest.py.new
grep "${PROJECT_NAME}" conftest.py.new
ln -s conftest.py.new conftest.py
pytest $@ .

