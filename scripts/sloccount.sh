#!/bin/sh

rm -rf sloccount_direct_bt
mkdir -p sloccount_direct_bt
cd sloccount_direct_bt

mkdir api
cd api
cp -a ../../api/direct_bt/* .
rm -fv *Ioctl.hpp
cd ..
# ln -s ../api/direct_bt api

ln -s ../include/cppunit cppunit
ln -s ../src/direct_bt src_cpp
ln -s ../test/direct_bt test
ln -s ../java/direct_bt src_java
ln -s ../java/jni/direct_bt src_jni
ln -s ../test src_test
cd ..

sloccount --follow --personcost 100000 --overhead 1.30 sloccount_direct_bt 2>&1 | tee sloccount_direct_bt-`date +%Y%m%d`.log
#sloccount --details --wide --follow --personcost 100000 --overhead 1.30 sloccount_direct_bt 2>&1 | tee sloccount_direct_bt-`date +%Y%m%d`.log
