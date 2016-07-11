Tiny Bluetooth LE Library
=============

[![Build Status](https://travis-ci.org/intel-iot-devkit/tinyb.svg?branch=master)](https://travis-ci.org/intel-iot-devkit/tinyb)
[![Coverity Scan](https://scan.coverity.com/projects/7546/badge.svg)](https://scan.coverity.com/projects/intel-iot-devkit-tinyb)

This project aims to create clean, modern and easy to use Bluetooth LE API.
TinyB exposes the BLE GATT API for C++, Java and other languages, using BlueZ
over DBus.

API Documentation
============

Up to date API documentation can be found:
* for C++: http://iotdk.intel.com/docs/master/tinyb/
* for Java: http://iotdk.intel.com/docs/master/tinyb/java/

A guide for getting started with TinyB on Java is available here:
https://software.intel.com/en-us/java-for-bluetooth-le-apps.

Using TinyB
============

TinyB requires CMake 3.1+ for building and requires GLib/GIO 2.40+. It also
requires BlueZ with GATT profile activated, which is currently experimental (as
of BlueZ 5.37), so you might have to run bluetoothd with the -E flag. For
example, on a system with systemd (Fedora, poky, etc.) edit the
bluetooth.service file (usually found in /usr/lib/systemd/system/ or
/lib/systemd/system) and append -E to ExecStart line, restart the daemon with
systemctl restart bluetooth.

~~~~~~~~~~~~~{.sh}
mkdir build
cd build
cmake ..
make
make install
~~~~~~~~~~~~~

The last command will create the include/ and lib/ directories with a copy of
the headers and library objects respectively in your build location. Note that
doing an out-of-source build may cause issues when rebuilding later on.

Our cmake configure has a number of options, *cmake-gui* or *ccmake* can show
you all the options. The interesting ones are detailed below:

Changing install path from /usr/local to /usr
~~~~~~~~~~~~~
-DCMAKE_INSTALL_PREFIX=/usr
~~~~~~~~~~~~~
Building debug build:
~~~~~~~~~~~~~
-DCMAKE_BUILD_TYPE=DEBUG
~~~~~~~~~~~~~
Using clang instead of gcc:
~~~~~~~~~~~~~
-DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++
~~~~~~~~~~~~~
Cross-compiling on a different system:
~~~~~~~~~~~~~
-DCMAKE_CXX_FLAGS:STRING=-m32 -march=i586
-DCMAKE_C_FLAGS:STRING=-m32 -march=i586
~~~~~~~~~~~~~
To build Java bindings:
~~~~~~~~~~~~~
-DBUILDJAVA=ON
~~~~~~~~~~~~~
To build documentation run: 
~~~~~~~~~~~~~
make doc
~~~~~~~~~~~~~

The hellotinyb example uses a [TI Sensor Tag](http://www.ti.com/ww/en/wireless_connectivity/sensortag2015/?INTC=SensorTag&HQS=sensortag)
from which it reads the ambient temperature. You have to pass the MAC address
of the Sensor Tag as a first parameter to the program.

Changes
============
  * 0.5.0
    ** Added notifications API
    ** Capitalized RSSI and UUID properly in Java
    ** Added JNI Helper classes for managing lifetime of JNIEnv and Global Refences
  * 0.4.0 - Added asynchronous methods for discovering BluetoothObjects


Common issues
============

If you have any issues, please go through the [Troubleshooting Guide](TROUBLESHOOTING.md). If the solution is not there, please create a new issue on [Github](https://github.com/intel-iot-devkit/tinyb).

Contributing to TinyB
============

You must agree to Developer Certificate of Origin and Sign-off your code,
using a real name and e-mail address. 
Please check the [Contribution](CONTRIBUTING.md) document for more details.
