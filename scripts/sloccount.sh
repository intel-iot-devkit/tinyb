#!/bin/sh

rm -rf sloccount_direct_bt
mkdir -p sloccount_direct_bt
cd sloccount_direct_bt
ln -s ../api/direct_bt api
ln -s ../include/cppunit cppunit
ln -s ../src/direct_bt src
ln -s ../test/direct_bt test
cd ..

sloccount --follow --personcost 100000 --overhead 1.30 sloccount_direct_bt 2>&1 | tee sloccount_direct_bt-`date +%Y%m%d`.log
