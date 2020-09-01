Tiny Bluetooth LE Library / Direct-BT LE and BREDR Library
==========================================================

Goals
============

This project aims to create a clean, modern and easy to use Bluetooth LE and BREDR API
fully accessible through C++, Java and other languages.


TinyB Version 2
================

Starting with version 2.1.0, the specification has slightly changed and hence its implementation.

As of today, the TinyB Java API comprises two implementations, *TinyB* and *Direct-BT*.


TinyB
-----
*TinyB* exposes the BLE GATT API for C++, Java and other languages, using BlueZ over DBus.

*TinyB* does not expose the BREDR API.

*TinyB* is exposed via the following native libraries
- *libtinyb.so* for the core C++ implementation.
- *libjavatinyb.so* for the Java binding.

*TinyB* is the original implementation of the TinyB project by Intel.


Direct-BT
----------
*Direct-BT* provides direct Bluetooth LE and BREDR programming without intermediate layers
targeting high-performance reliable Bluetooth support.

*Direct-BT* may be utilized via its C++ API or via the TinyB Java API.

By having least system and userspace dependencies and no communication overhead, 
Direct-BT shall be suitable for embedded device configurations besides others.

*Direct-BT* is exposed via the following native libraries
- *libdirect_bt.so* for the core C++ implementation.
- *libjavadirect_bt.so* for the Java binding.

To use *Direct-BT* in the most efficient way, 
the BlueZ userspace daemon *bluetoothd* should be disabled.
Using systemd this should be:

```
systemctl stop bluetooth
systemctl disable bluetooth
systemctl mask bluetooth
```

You will find a detailed overview of *Direct-BT* in the doxygen generated 
[C++ API doc of its *direct_bt* namespace](../../cpp/html/namespacedirect__bt.html#details).

*Direct-BT* is the new implementation as provided by [Zafena ICT](https://ict.zafena.se).


TinyB and Direct-BT
-------------------
Pre version 2.0.0 D-Bus implementation details of the Java[tm] classes
of package *tinyb* has been moved to *tinyb.dbus*.
The *tinyb.jar* jar file has been renamed to *tinyb2.jar*, avoiding conflicts.

General interfaces matching the original implementation 
and following [BlueZ API](http://git.kernel.org/cgit/bluetooth/bluez.git/tree/doc/)
were created in package *org.tinyb*.

*org.tinyb.BluetoothFactory* provides a factory to instantiate the initial root
*org.tinyb.BluetoothManager*, either using *Tiny-B*, the original D-Bus implementation,
or *Direct-BT*, the direct implementation.

*TinyB*'s C++ namespace and implementation kept mostly unchanged.

The new Java interface of package *org.tinyb* has been kept mostly compatible,
however, changes were required to benefit from *Direct-BT*'s implementation.

*since 2.x* version tags have been added to the Java interface specification for clarity.


API Documentation
============

Up to date API documentation can be found:
* [C++ API Doc](../../cpp/html/index.html).
    * [Overview of *direct_bt*](../../cpp/html/namespacedirect__bt.html#details).
* [Java API Doc](../../java/html/index.html).

A guide for getting started with Direct-BT on C++ and Java will follow up soon from Zafena ICT.

A guide for getting started with TinyB on Java is available from Intel:
https://software.intel.com/en-us/java-for-bluetooth-le-apps.

Build Status
============

*Will be updated soon*


Using TinyB / Direct-BT
=========================

The project requires CMake 3.1+ for building and a Java JDK >= 11.

*TinyB* requires GLib/GIO 2.40+. It also
requires BlueZ with GATT profile activated, which is currently experimental (as
of BlueZ 5.37), so you might have to run bluetoothd with the -E flag. For
example, on a system with systemd (Fedora, poky, etc.) edit the
bluetooth.service file (usually found in /usr/lib/systemd/system/ or
/lib/systemd/system) and append -E to ExecStart line, restart the daemon with
systemctl restart bluetooth.


*Direct-BT* does not require GLib/GIO 
nor shall the BlueZ userspace service *bluetoothd* be active for best experience.

To disable the *bluetoothd* service using systemd:
```
systemctl stop bluetooth
systemctl disable bluetooth
systemctl mask bluetooth
```

For a generic build use:
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
To not build the *TinyB* implementation:
~~~~~~~~~~~~~
-DSKIP_TINYB=ON
~~~~~~~~~~~~~
To build examples:
~~~~~~~~~~~~~
-DBUILDEXAMPLES=ON
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
- 2.0.0
  - Java D-Bus implementation details of package 'tinyb' moved to *tinyb.dbus*.
  - The *tinyb.jar* jar file has been renamed to *tinyb2.jar*, avoiding conflicts.
  - General interfaces matching the original implementation and following [BlueZ API](http://git.kernel.org/cgit/bluetooth/bluez.git/tree/doc/device-api.txt)
were created in package *org.tinyb*.
  - Class *org.tinyb.BluetoothFactory* provides a factory to instantiate the initial root *org.tinyb.BluetoothManager*, either using the original D-Bus implementation or an alternative implementation.
  - C++ namespace and implementation kept unchanged.
- 0.5.0
  - Added notifications API
  - Capitalized RSSI and UUID properly in Java
  - Added JNI Helper classes for managing lifetime of JNIEnv and Global Refences
- 0.4.0 
  - Added asynchronous methods for discovering BluetoothObjects

Common issues
============

If you have any issues, please go through the [Troubleshooting Guide](TROUBLESHOOTING.md). 
If the solution is not there, please create a new issue on [Zafena ICT](https://ict.zafena.se/issues/).

Contributing to TinyB / Direct-BT
===================================

You shall agree to Developer Certificate of Origin and Sign-off your code,
using a real name and e-mail address. 
Please check the [Contribution](CONTRIBUTING.md) document for more details.
